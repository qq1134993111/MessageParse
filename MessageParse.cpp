#include "MessageParse.h"

#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/xml_parser.hpp"
#include "boost/optional.hpp"
#include "boost/typeof/typeof.hpp"
#include "boost/foreach.hpp"
#include "boost/exception/all.hpp"
#include "boost/filesystem.hpp"
#include "inja/inja.hpp"
#include "boost/algorithm/string.hpp"
#include"FileUtil.h"
#include<unordered_set>

bool MessageParser::LoadXml(const std::string& file_path)
{
    try
    {
        boost::filesystem::path path(file_path);
        file_name_ = path.stem().string();

        boost::property_tree::ptree root;
        boost::property_tree::read_xml(file_path, root);

        auto types_tree = root.get_child("File.Types");
        boost::property_tree::ptree& messages_tree = root.get_child("File.Messages");
        boost::optional<boost::property_tree::ptree&> constants_tree = root.get_child_optional("File.Constants");

        endian_ = (EndianType)root.get<int32_t>("File.<xmlattr>.Endian");

        std::string str_namespace = root.get<std::string>("File.<xmlattr>.namespace");
        boost::algorithm::split(v_namespace_, str_namespace, boost::is_any_of("."), boost::token_compress_on);

        //类型信息
        for (BOOST_AUTO(pos, types_tree.begin()); pos != types_tree.end(); ++pos)
        {
            std::cout << fmt::format("Types tag:{0},data:{1}\n", pos->first, pos->second.get_value(""));

            if (pos->first != "Type")
            {
                std::cout << fmt::format("tag {} not Type,ignore.\n", pos->first);
                continue;
            }

            auto name = pos->second.get<std::string>("<xmlattr>.name");
            auto primitive_type = pos->second.get<std::string>("<xmlattr>.primitive_type");
            auto description = pos->second.get<std::string>("<xmlattr>.description");
            auto len = pos->second.get_optional<int32_t>("<xmlattr>.length");

            std::cout << fmt::format("name:{},primitive_type:{},description:{},len:{}\n", name, primitive_type, description, len.value_or(0));

            TypeInfoBase type_info(name, primitive_type, len.value_or(0), description);

            //检测类型
            if (!TypeRecognition::IsPrimitiveTypeValid(type_info.GetPrimitiveType()))
            {
                std::cout << fmt::format("type name {0} primitive_type {1} not valid.\n", type_info.GetName(), type_info.GetPrimitiveType());
                return false;
            }

            //数组类型必须有长度
            if (TypeRecognition::IsPrimitiveTypeFixArray(primitive_type))
            {
                if (!len)
                {
                    std::cout << fmt::format("type name {0} primitive_type {1} length not valid,must set value.\n", name, primitive_type);
                    return false;
                }

                if (len.value() <= 0)
                {
                    std::cout << fmt::format("type name {0} primitive_type {1} length is {2},must >= 0\n", name, primitive_type, len.value());
                    return false;
                }
            }
            else if (TypeRecognition::IsPrimitiveTypeString(primitive_type))
            {
                len = std::numeric_limits<int32_t>::max();
            }
            else
            {
                len = TypeRecognition::GetPrimitiveTypeIntSize(primitive_type);
            }

            //名称不能重复
            if (type_info_map_.count(type_info.GetName()))
            {
                std::cout << fmt::format("File.Types duplicate key ,{} has been defined.\n", type_info.GetName());
                return false;
            }

            type_info_map_.insert(std::make_pair(type_info.GetName(), type_info));
            v_type_info_.push_back(type_info);
        }

        //消息信息
        BOOST_FOREACH(boost::property_tree::ptree::value_type & v1, messages_tree)
        {
            std::cout << fmt::format("Messages tag:{0},data:{1}\n", v1.first, v1.second.get_value(""));
            std::string msg_name = v1.second.get<std::string>("<xmlattr>.name");
            auto pktno = v1.second.get_optional<uint32_t>("<xmlattr>.pktno");
            auto inherit = v1.second.get_optional<std::string>("<xmlattr>.inherit");
            std::string description = v1.second.get<std::string>("<xmlattr>.description");

            std::cout << fmt::format("msg_name:{},pktno:{},inherit:{},description:{}\n", msg_name, pktno.value_or(-1), inherit.value_or(""), description);

            if (inherit)
            {
                //获取父类消息
                std::cout << fmt::format("msg_name {0} inherit {1}\n", msg_name, inherit.value());
                if (!msg_name_struct_map_.count(*inherit))
                {
                    std::cout << fmt::format("message {} cannot find inherit {}.\n", msg_name, *inherit);
                    return false;
                }
            }

            //是否有消息名称重复
            if (msg_name_struct_map_.count(msg_name))
            {
                std::cout << fmt::format("The message duplicate key,{} has been defined.\n", msg_name);
                return false;
            }

            if (pktno)
            {
                //是否消息号重复
                if (msg_pktno_struct_map_.count(pktno.value()))
                {
                    std::cout << fmt::format("The message {} pktno duplicate key, {} has been defined.\n", msg_name, pktno.value());
                    return false;
                }
            }

            MessageInfoBase msg_info(msg_name, pktno.value_or(0), inherit.value_or(""), description);

            //遍历保存field
            for (auto& v2 : v1.second)
            {
                std::cout << fmt::format("Field tag:{0},data:{1}\n", v2.first, v2.second.get_value(""));
                if (v2.first == "<xmlattr>")
                {
                    std::cout << "<xmlattr> continue\n";
                    continue;
                }

                std::string field_name = v2.second.get<std::string>("<xmlattr>.name");
                std::string primitive_type = v2.second.get<std::string>("<xmlattr>.primitive_type");
                std::string description = v2.second.get<std::string>("<xmlattr>.description");
                auto len = v2.second.get_optional<int32_t>("<xmlattr>.length");

                std::cout << fmt::format("field_name:{},primitive_type:{},description:{},len:{}\n", field_name, primitive_type, description, len.value_or(0));

                if (v2.first == "Field")
                {
                    //无效的类型
                    if (!type_info_map_.count(primitive_type) && !TypeRecognition::IsPrimitiveTypeValid(primitive_type))
                    {
                        std::cout << fmt::format("The message {} field {} primitive_type {} cannot find.\n", msg_name, field_name, primitive_type);
                        return false;
                    }

                    //数组长度检验
                    if (TypeRecognition::IsPrimitiveTypeFixArray(primitive_type))
                    {
                        if (!len)
                        {
                            std::cout << fmt::format("The message {0} field {1} primitive_type {2} length not valid,must set value.\n", msg_name, field_name, primitive_type);
                            return false;
                        }

                        if (len.value() <= 0)
                        {
                            std::cout << fmt::format("The message {0} field {1} primitive_type {2} length is {3},must >= 0\n", msg_name, field_name, primitive_type, len.value());
                            return false;
                        }
                    }
                    else if (TypeRecognition::IsPrimitiveTypeString(primitive_type))
                    {
                        len = std::numeric_limits<int32_t>::max();
                    }
                    else
                    {
                        len = TypeRecognition::GetPrimitiveTypeIntSize(primitive_type);
                    }

                    FieldInfoBase simple_info(FieldType::Primitive, field_name, primitive_type, len.value_or(0), description);
                    msg_info.PushFiled(simple_info);
                }
                else if (v2.first == "Sequence")
                {
                    if (msg_name_struct_map_.count(primitive_type) || type_info_map_.count(primitive_type) || TypeRecognition::IsPrimitiveTypeValid(primitive_type))
                    {

                        if (TypeRecognition::IsPrimitiveTypeFixArray(primitive_type))
                        {
                            if (!len)
                            {
                                std::cout << fmt::format("The message {0} field {1} primitive_type {2} length not valid,must set value.\n", msg_name, field_name, primitive_type);
                                return false;
                            }

                            if (len.value() <= 0)
                            {
                                std::cout << fmt::format("The message {0} field {1} primitive_type {2} length is {3},must >= 0\n", msg_name, field_name, primitive_type, len.value());
                                return false;
                            }
                        }
                        else if (TypeRecognition::IsPrimitiveTypeString(primitive_type))
                        {
                            len = std::numeric_limits<int32_t>::max();
                        }
                        else
                        {
                            len = TypeRecognition::GetPrimitiveTypeIntSize(primitive_type);
                        }

                        FieldInfoBase struct_info(FieldType::Sequence, field_name, primitive_type, len.value_or(0), description);
                        msg_info.PushFiled(struct_info);
                    }
                    else
                    {
                        std::cout << fmt::format("The message {} Sequence tag primitive_type {} cannot find.\n", msg_name, primitive_type);
                        return false;
                    }

                }
                else
                {
                    std::cout << fmt::format("The message {} unkown field tag {}\n", msg_name, v2.first);
                    return false;
                }
            }


            msg_name_struct_map_[msg_name] = msg_info;
            if (pktno)
            {
                msg_pktno_struct_map_[*pktno] = msg_info;
            }
            v_msg_struct_info_.push_back(msg_info);

        }

        //常量
        if (constants_tree)
        {
            for (auto& v1 : *constants_tree)
            {
                std::cout << fmt::format("Const tag:{0},data:{1}\n", v1.first, v1.second.get_value(""));

                std::string name = v1.second.get<std::string>("<xmlattr>.name");
                auto primitive_type = v1.second.get<std::string>("<xmlattr>.primitive_type");
                std::string description = v1.second.get<std::string>("<xmlattr>.description");
                auto len = v1.second.get_optional<int32_t>("<xmlattr>.length");

                std::cout << fmt::format("name:{},primitive_type:{},description:{},len:{}\n", name, primitive_type, description, len.value_or(0));

                //获取最原始的类型
                std::string original_type = "";
                int32_t original_len = 0;


                auto it = type_info_map_.find(primitive_type);
                if (it != type_info_map_.end())
                {
                    original_type = it->second.GetPrimitiveType();
                    original_len = it->second.GetLength();
                }
                else if (TypeRecognition::IsPrimitiveTypeValid(primitive_type))
                {
                    original_type = primitive_type;
                    if (TypeRecognition::IsPrimitiveTypeFixArray(primitive_type))
                    {
                        if (!len)
                        {
                            std::cout << fmt::format("The Const {0} primitive_type {1} length not valid,must set value.\n", name, primitive_type);
                            return false;
                        }

                        if (len.value() <= 0)
                        {
                            std::cout << fmt::format("The Const {0} primitive_type {1} length is {2},must >= 0\n", name, primitive_type, len.value());
                            return false;
                        }
                        original_len = len.value();
                    }
                    else if (TypeRecognition::IsPrimitiveTypeString(original_type))
                    {
                        len = std::numeric_limits<int32_t>::max();
                    }
                    else
                    {
                        len = TypeRecognition::GetPrimitiveTypeIntSize(primitive_type);
                    }
                }
                else
                {
                    std::cout << fmt::format("The const name {} primitive_type {} cannot find.\n", name, primitive_type);
                    return false;
                }


                ConstInfoBase const_info(name, primitive_type, len.value_or(0), description);
                for (auto& v2 : v1.second)
                {
                    if (v2.first == "<xmlattr>")
                        continue;

                    std::cout << fmt::format("Const value tag:{0},data:{1}\n", v2.first, v2.second.get_value(""));
                    std::string field_name = v2.second.get<std::string>("<xmlattr>.name");
                    std::string description = v2.second.get<std::string>("<xmlattr>.description");
                    std::string value = v2.second.get<std::string>("");
                    //判断值是否合法

                    boost::algorithm::trim(value);

                    if (TypeRecognition::IsPrimitiveTypeInt(original_type))
                    {
                        bool b_valid = false;
                        if (original_type == "CHAR")
                        {
                            if (value.size() == 1)
                            {
                                b_valid = true;
                            }
                        }
                        else if (original_type == "BOOL")
                        {
                            if (value == "true" || value == "false")
                            {
                                b_valid = true;
                            }
                            else
                            {
                                try
                                {
                                    auto v = std::stoll(value);
                                    b_valid = true;
                                }
                                catch (const std::exception&)
                                {
                                    b_valid = false;
                                }
                            }
                        }
                        else
                        {
                            try
                            {
                                if (original_type == "INT8")
                                {
                                    auto v = std::stoll(value);
                                    b_valid = (v >= std::numeric_limits<int8_t>::min() && v <= std::numeric_limits<int8_t>::max());
                                }
                                else if (original_type == "UCHAR" || original_type == "UINT8")
                                {
                                    auto v = std::stoull(value);
                                    b_valid = (v >= std::numeric_limits<uint8_t>::min() && v <= std::numeric_limits<uint8_t>::max());
                                }
                                else if (original_type == "INT16")
                                {
                                    auto v = std::stoll(value);
                                    b_valid = (v >= std::numeric_limits<int16_t>::min() && v <= std::numeric_limits<int16_t>::max());
                                }
                                else if (original_type == "UINT16")
                                {
                                    auto v = std::stoull(value);
                                    b_valid = (v >= std::numeric_limits<uint16_t>::min() && v <= std::numeric_limits<uint16_t>::max());
                                }
                                else if (original_type == "INT32")
                                {
                                    auto v = std::stoll(value);
                                    b_valid = (v >= std::numeric_limits<int32_t>::min() && v <= std::numeric_limits<int32_t>::max());
                                }
                                else if (original_type == "UINT32")
                                {
                                    auto v = std::stoull(value);
                                    b_valid = (v >= std::numeric_limits<uint32_t>::min() && v <= std::numeric_limits<uint32_t>::max());
                                }
                                else if (original_type == "INT64")
                                {
                                    auto v = std::stoll(value);
                                    b_valid = (v >= std::numeric_limits<int64_t>::min() && v <= std::numeric_limits<int64_t>::max());
                                }
                                else if (original_type == "UINT64")
                                {
                                    auto v = std::stoull(value);
                                    b_valid = (v >= std::numeric_limits<uint64_t>::min() && v <= std::numeric_limits<uint64_t>::max());
                                }
                            }
                            catch (const std::exception&)
                            {
                                b_valid = false;
                            }
                        }

                        if (!b_valid)
                        {
                            std::cout << fmt::format("const name{} field{} primitive_type{} is not valid, value{}\n",
                                name, field_name, primitive_type, value);

                            return false;
                        }
                    }
                    else if (TypeRecognition::IsPrimitiveTypeFixArray(original_type))
                    {
                        if (value.size() > original_len)
                        {
                            std::cout << fmt::format("const name {} field {} primitive_type {} is not valid,value {} length {} > defined length {}\n",
                                name, field_name, primitive_type, value, value.size(), original_len);

                            return false;
                        }
                    }

                    FieldInfoValue field(value, field_name, primitive_type, len.value_or(0), description);

                    const_info.PushFiled(field);
                }

                if (const_name_map_.count(name))
                {
                    std::cout << fmt::format("const name  duplicate key,{} has been defined.\n", name);
                    return false;
                }

                const_name_map_[name] = const_info;
                v_const_info_.push_back(const_info);

            }
        }

    }
    catch (...)
    {
        std::cout << fmt::format("load {} exception,{}", file_path, boost::current_exception_diagnostic_information()) << "\n";
    }

    return true;
}

bool MessageParser::Write(const std::string& template_path, const std::string& write_path)
{
    try
    {
        bool revised_type = true;
        boost::filesystem::create_directories(write_path);
        inja::Environment env{ template_path + "\\",write_path + "\\" };
        env.add_callback("RevisedType", 2,
            [&](inja::Arguments& args)
            {
                std::string type_name = args.at(0)->get<std::string>();
                if (revised_type)
                {
                    static	std::unordered_map<std::string, std::string>  s_umap
                    {
                        {"CHAR","char"},
                        {"UCHAR","unsigned char"},
                        {"BOOL","bool"},
                        {"INT8","int8_t"},
                        {"UINT8","uint8_t"},
                        {"INT16","int16_t" },
                        {"UINT16","uint16_t"},
                        {"INT32","int32_t"},
                        {"UINT32","uint32_t"},
                        {"INT64","int64_t"},
                        {"UINT64","uint64_t"},
                        {"STRING","std::string"}
                    };

                    auto it = s_umap.find(type_name);
                    if (it != s_umap.end())
                        return it->second;
                }

                if (type_name == "FIXARRAY")
                {
                    return fmt::format("std::array<char,{}>", args.at(1)->get<uint32_t>());
                }

                return type_name;

            });
        env.set_trim_blocks(true); //将删除语句后的第一个换行符
        env.set_lstrip_blocks(true);//

        //类型定义
        if (1)
        {

            std::cout << fmt::format("parse TEMPLATE_MESSAGE_H\n");
            inja::Template temp_msg_h = env.parse_template("TEMPLATE_MESSAGE_H.txt");
            std::cout << fmt::format("parse TEMPLATE_MESSAGE_CPP\n");
            inja::Template temp_msg_cpp = env.parse_template("TEMPLATE_MESSAGE_CPP.txt");

            for (auto& [key, value] : msg_name_struct_map_)
            {
                inja::json json;
                json["NAMESPACE"] = v_namespace_;
                json["MSG_DESCRIPTION"] = value.GetDescription();
                json["MSG_INHERIT"] = value.GetInherit();
                json["MSG_PKT_NO"] = value.GetPktNo();
                json["MSG_NAME"] = value.GetName();

                auto field_info = value.GetFields();

                for (auto& f : field_info)
                {
                    inja::json j_field;
                    j_field["F_DESCRIPTION"] = f.GetDescription();
                    j_field["F_FILED_TYPE"] = f.GetFiledType();
                    j_field["F_NAME"] = f.GetName();
                    j_field["F_PRIMITIVE_TYPE"] = f.GetPrimitiveType();
                    j_field["F_LENGTH"] = f.GetLength();
                    auto it = type_info_map_.find(f.GetPrimitiveType());
                    if (it != type_info_map_.end())
                    {
                        if (TypeRecognition::IsPrimitiveTypeInt(it->second.GetPrimitiveType()))
                        {
                            j_field["F_TYPE_INFO"] =
                            {
                                {"T_NAME",type_info_map_[it->second.GetPrimitiveType()].GetName()},
                                {"T_PRIMITIVE_TYPE",type_info_map_[it->second.GetPrimitiveType()].GetPrimitiveType()},
                                {"T_LENGTH",type_info_map_[it->second.GetPrimitiveType()].GetLength()}
                            };

                        }
                        else//string fixarray
                        {
                            j_field["F_TYPE_INFO"] =
                            {
                                {"T_NAME",it->second.GetName()},
                                {"T_PRIMITIVE_TYPE",it->second.GetPrimitiveType()},
                                {"T_LENGTH",it->second.GetLength()}
                            };
                        }
                    }
                    else//field 中直接 FIXARRAY
                    {
                        j_field["F_TYPE_INFO"] =
                        {
                            {"T_NAME",f.GetPrimitiveType()},
                            {"T_PRIMITIVE_TYPE",f.GetPrimitiveType()},
                            {"T_LENGTH",f.GetLength()}
                        };
                    }


                    json["FIELDS"].push_back(j_field);
                }

                std::cout << fmt::format("write {}.h\n", key);
                env.write(temp_msg_h, json, key + ".h");
                std::cout << fmt::format("write {}.cpp\n", key);
                env.write(temp_msg_cpp, json, key + ".cpp");

            }
        }

        //消息号定义
        if (1)
        {
            std::cout << fmt::format("parse TEMPLATE_MESSAGE_TYPES_DEFINITION_H.txt\n");
            inja::Template temp_msg_h = env.parse_template("TEMPLATE_MESSAGE_TYPES_DEFINITION_H.txt");

            inja::json types_json;
            types_json["NAMESPACE"] = v_namespace_;
            for (auto& msg_info : v_msg_struct_info_)
            {
                if (msg_info.GetPktNo() == 0)
                    continue;

                inja::json json;
                //json["NAMESPACE"] = v_namespace;
                json["MSG_DESCRIPTION"] = msg_info.GetDescription();
                json["MSG_INHERIT"] = msg_info.GetInherit();
                json["MSG_PKT_NO"] = msg_info.GetPktNo();
                json["MSG_NAME"] = msg_info.GetName();

                types_json["MSG_INFOS"].push_back(json);
            }

            std::string write_file_name = "MessageTypesDefinition.h";
            std::cout << fmt::format("write {}\n", write_file_name);
            env.write(temp_msg_h, types_json, write_file_name);
        }

        //工厂定义
        if (1)
        {
         
            env.add_callback("GenNamespacePrefix",
                [&](inja::Arguments& args)
                { 
                    auto v_space = args.at(0)->get<std::vector<std::string>>();
                    std::string s;
                    for (auto& v : v_space)
                    {
                        s += v + "::";
                    }
                    return s;     //A::B::
                });

            std::cout << fmt::format("parse TEMPLATE_FACTORY_H.txt\n");
            inja::Template temp_msg_h = env.parse_template("TEMPLATE_FACTORY_H.txt");

       

            inja::json json;
            json["NAMESPACE"] = v_namespace_;
            json["FILENAME"] = file_name_;


            std::string write_file_name = "MessageFactory.h";
            std::cout << fmt::format("write {}\n", write_file_name);
            env.write(temp_msg_h, json, write_file_name);
        }
        //工厂注册
        if (1)
        {
            std::cout << fmt::format("parse TEMPLATE_FACTORY_REGISTER_CPP.txt\n");
            inja::Template temp_factory_register_cpp = env.parse_template("TEMPLATE_FACTORY_REGISTER_CPP.txt");

            inja::json types_json;
            types_json["NAMESPACE"] = v_namespace_;
            types_json["FILENAME"] = file_name_;


            for (auto& msg_info : v_msg_struct_info_)
            {
                if (msg_info.GetPktNo() == 0)
                    continue;

                inja::json json;
                //json["NAMESPACE"] = v_namespace;
                json["MSG_DESCRIPTION"] = msg_info.GetDescription();
                json["MSG_INHERIT"] = msg_info.GetInherit();
                json["MSG_PKT_NO"] = msg_info.GetPktNo();
                json["MSG_NAME"] = msg_info.GetName();

                types_json["MSG_INFOS"].push_back(json);
            }

            std::string write_file_name = "MessageFactoryRegister.cpp";
            std::cout << fmt::format("write {}\n", write_file_name);
            env.write(temp_factory_register_cpp, types_json, write_file_name);
        }


        //类型定义
        if (1)
        {
            std::cout << fmt::format("parse TEMPLATE_TYPES_DEFINITION_H\n");
            inja::Template temp_types_h = env.parse_template("TEMPLATE_TYPES_DEFINITION_H.txt");
            inja::json json_types;
            json_types["NAMESPACE"] = v_namespace_;
            json_types["TYPES"].push_back({ {"T_NAME","CHAR"},{"T_PRIMITIVE_TYPE","CHAR"},{"T_LENGTH",1},{"T_DESCRIPTION","CHAR"} });
            json_types["TYPES"].push_back({ {"T_NAME","UCHAR"},{"T_PRIMITIVE_TYPE","UCHAR"},{"T_LENGTH",1},{"T_DESCRIPTION","UNSIGNED CHAR"} });
            json_types["TYPES"].push_back({ {"T_NAME","BOOL"},{"T_PRIMITIVE_TYPE","BOOL"},{"T_LENGTH",1},{"T_DESCRIPTION","BOOL"} });
            json_types["TYPES"].push_back({ {"T_NAME","INT8"},{"T_PRIMITIVE_TYPE","INT8"},{"T_LENGTH",1},{"T_DESCRIPTION","INT8_T"} });
            json_types["TYPES"].push_back({ {"T_NAME","UINT8"},{"T_PRIMITIVE_TYPE","UINT8"},{"T_LENGTH",1},{"T_DESCRIPTION","UINT8_T"} });
            json_types["TYPES"].push_back({ {"T_NAME","INT16"},{"T_PRIMITIVE_TYPE","INT16"},{"T_LENGTH",2},{"T_DESCRIPTION","INT8_T"} });
            json_types["TYPES"].push_back({ {"T_NAME","UINT16"},{"T_PRIMITIVE_TYPE","UINT16"},{"T_LENGTH",2},{"T_DESCRIPTION","UINT16_T"} });
            json_types["TYPES"].push_back({ {"T_NAME","INT32"},{"T_PRIMITIVE_TYPE","INT32"},{"T_LENGTH",4},{"T_DESCRIPTION","INT32_T"} });
            json_types["TYPES"].push_back({ {"T_NAME","UINT32"},{"T_PRIMITIVE_TYPE","UINT32"},{"T_LENGTH",4},{"T_DESCRIPTION","UINT32_T"} });
            json_types["TYPES"].push_back({ {"T_NAME","INT64"},{"T_PRIMITIVE_TYPE","INT64"},{"T_LENGTH",8},{"T_DESCRIPTION","INT64_T"} });
            json_types["TYPES"].push_back({ {"T_NAME","UINT64"},{"T_PRIMITIVE_TYPE","UINT64"},{"T_LENGTH",8},{"T_DESCRIPTION","UINT64_T"} });
            json_types["TYPES"].push_back({ {"T_NAME","STRING"},{"T_PRIMITIVE_TYPE","STRING"},{"T_LENGTH",std::numeric_limits<int32_t>::max()},{"T_DESCRIPTION","STD::STRING"} });

            for (auto& [key, value] : type_info_map_)
            {
                static std::unordered_set<std::string> s_set{ "BOOL","CHAR","UCHAR","INT8","UINT8","INT16","UINT16","INT32","UINT32","INT64","UINT64","STRING" };
                if (s_set.count(key))
                    continue;

                std::string name_;
                std::string primitive_type_;
                int32_t  length_ = 0;
                std::string description_;

                json_types["TYPES"].push_back(
                    {
                    {"T_NAME",value.GetName()},
                    {"T_PRIMITIVE_TYPE",value.GetPrimitiveType()},
                    {"T_LENGTH",value.GetLength()},
                    {"T_DESCRIPTION",value.GetDescription()}
                    });

            }

            std::cout << json_types << "\n";

            std::cout << fmt::format("write TypesDefinition.h\n");
            env.write(temp_types_h, json_types, "TypesDefinition.h");
        }

        //常量定义
        if (1)
        {
            std::cout << fmt::format("parse TEMPLATE_CONSTANTS_H\n");
            inja::Template temp_constants_h = env.parse_template("TEMPLATE_CONSTANTS_H.txt");
            std::cout << fmt::format("parse TEMPLATE_CONSTANTS_CPP\n");
            inja::Template temp_constants_cpp = env.parse_template("TEMPLATE_CONSTANTS_CPP.txt");
            inja::json json_constants;
            json_constants["NAMESPACE"] = v_namespace_;

            for (auto& [key, value] : const_name_map_)
            {
                inja::json json;
                json["CONST_DESCRIPTION"] = value.GetDescription();
                json["CONST_NAME"] = value.GetName();
                json["CONST_PRIMITIVE_TYPE"] = value.GetPrimitiveType();
                json["CONST_LENGTH"] = value.GetLength();
                auto field_info = value.GetFields();

                for (auto& f : field_info)
                {
                    inja::json j_field;
                    j_field["F_VALUE"] = f.GetValue();
                    j_field["F_DESCRIPTION"] = f.GetDescription();
                    j_field["F_NAME"] = f.GetName();
                    j_field["F_PRIMITIVE_TYPE"] = f.GetPrimitiveType();
                    j_field["F_LENGTH"] = f.GetLength();

                    auto it = type_info_map_.find(f.GetPrimitiveType());
                    if (it != type_info_map_.end())
                    {
                        j_field["F_TYPE_INFO"] =
                        {
                            {"T_NAME",it->second.GetName()},
                            {"T_PRIMITIVE_TYPE",it->second.GetPrimitiveType()},
                            {"T_LENGTH",it->second.GetLength()}
                        };
                    }
                    else//field 中直接 FIXARRAY
                    {
                        j_field["F_TYPE_INFO"] =
                        {
                            {"T_NAME", value.GetPrimitiveType() },
                            {"T_PRIMITIVE_TYPE", value.GetPrimitiveType()},
                            {"T_LENGTH",value.GetLength()}
                        };
                    }

                    json["FIELDS"].push_back(j_field);
                }
                //std::cout << json << "\n";
                json_constants["CONSTANTS"].push_back(json);
            }

            //std::cout << json_constants << "\n";
            //std::string line = json_constants.dump();
            //FileUtil::WriteFile("relsut", std::vector<std::string>{line});

            std::cout << fmt::format("write Constants.h\n");
            env.write(temp_constants_h, json_constants, "Constants.h");
            std::cout << fmt::format("write Constants.cpp\n");
            env.write(temp_constants_cpp, json_constants, "Constants.cpp");
            int i;
            i = 0;

        }

    }
    catch (const std::exception& e)
    {
        std::cout << "Template write exception::" << e.what() << "\n";
        return false;
    }



    return false;
}
