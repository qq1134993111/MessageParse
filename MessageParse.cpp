#include "MessageParse.h"

#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/xml_parser.hpp"
#include "boost/optional.hpp"
#include "boost/typeof/typeof.hpp"
#include "boost/foreach.hpp"
#include "boost/exception/all.hpp"
#include "boost/filesystem.hpp"
#include "inja/inja.hpp"

#include<unordered_set>

bool MessageParser::LoadXml(const std::string& file_path)
{
	try
	{
		boost::property_tree::ptree root;
		boost::property_tree::read_xml(file_path, root);

		auto types_tree = root.get_child("File.Types");
		boost::property_tree::ptree& messages_tree = root.get_child("File.Messages");
		boost::optional<boost::property_tree::ptree&> constants_tree = root.get_child_optional("File.Constants");

		endian_ = (EndianType)root.get<int32_t>("File.<xmlattr>.Endian");


		//类型信息
		for (BOOST_AUTO(pos, types_tree.begin()); pos != types_tree.end(); ++pos)
		{
			std::cout << fmt::format("tag:{0},data:{1}\n", pos->first, pos->second.data());

			if (pos->first != "Type")
			{
				std::cout << fmt::format("tag {} not Type,ignore.\n");
				continue;
			}

			auto name = pos->second.get<std::string>("<xmlattr>.name");
			auto primitive_type = pos->second.get<std::string>("<xmlattr>.primitive_type");
			auto description = pos->second.get<std::string>("<xmlattr>.description");
			auto len = pos->second.get_optional<int32_t>("<xmlattr>.length");


			if (TypeRecognition::IsPrimitiveTypeFixArray(primitive_type))
			{
				if (!len)
				{
					//std::cout << "primitive_type length error:" << type_info.primitive_type << "\n";
					std::cout << fmt::format("type name {0} primitive_type {1} length not valid,must set value.", name, primitive_type);
					return false;
				}

				if (len.value() <= 0)
				{
					std::cout << fmt::format("type name {0} primitive_type {1} length is {2},must >= 0", name, primitive_type, len.value());
					return false;
				}
			}
			else if (TypeRecognition::IsPrimitiveTypeString(primitive_type))
			{
				len = std::numeric_limits<uint32_t>::max();
			}
			else
			{
				len = 0;
			}


			TypeInfoBase type_info(name, primitive_type, len.value_or(0), description);
			if (!TypeRecognition::IsPrimitiveTypeValid(type_info.GetPrimitiveType()))
			{
				std::cout << fmt::format("type name {0} primitive_type {1} not valid.\n", type_info.GetName(), type_info.GetPrimitiveType());
				return false;
			}

			if (type_info_map_.count(type_info.GetName()))
			{
				std::cout << fmt::format("File.Types duplicate key ,{} has been defined.\n", type_info.GetName());
				return false;
			}

			type_info_map_.insert(std::make_pair(type_info.GetName(), type_info));
		}

		//消息信息
		BOOST_FOREACH(boost::property_tree::ptree::value_type & v1, messages_tree)
		{
			std::cout << v1.first << "," << v1.second.data() << "\n";
			std::string msg_name = v1.second.get<std::string>("<xmlattr>.name");
			auto pktno = v1.second.get_optional<uint32_t>("<xmlattr>.pktno");
			auto inherit = v1.second.get_optional<std::string>("<xmlattr>.inherit");
			std::string description = v1.second.get<std::string>("<xmlattr>.description");

			if (inherit)
			{
				//获取父类消息
				std::cout << fmt::format("msg_name {0} inherit {1}", msg_name, inherit.value());
				if (!msg_name_struct_map_.count(*inherit))
				{
					std::cout << fmt::format("message {} cannot find inherit {}.\n", msg_name, *inherit);
					return false;
				}
			}

			if (msg_name_struct_map_.count(msg_name))
			{
				std::cout << fmt::format("The message duplicate key,{} has been defined.\n", msg_name);
				return false;
			}

			if (pktno)
			{
				if (msg_pktno_struct_map_.count(pktno.value()))
				{
					std::cout << fmt::format("The message {} pktno duplicate key, {} has been defined.\n", msg_name, pktno.value());
					return false;
				}
			}

			MessageInfoBase msg_info(msg_name, pktno.value_or(0), inherit.value_or(""), description);

			for (auto& v2 : v1.second)
			{
				if (v2.first == "<xmlattr>")
					continue;

				std::cout << "message field:" << v2.first << "\n";
				std::string field_name = v2.second.get<std::string>("<xmlattr>.name");
				std::string primitive_type = v2.second.get<std::string>("<xmlattr>.primitive_type");
				std::string description = v2.second.get<std::string>("<xmlattr>.description");
				auto len = v2.second.get_optional<int32_t>("<xmlattr>.length");

				if (v2.first == "Field")
				{
					if (!type_info_map_.count(primitive_type) && !TypeRecognition::IsPrimitiveTypeValid(primitive_type))
					{
						std::cout << fmt::format("The message {} field {} primitive_type {} cannot find.\n", msg_name, field_name, primitive_type);
						return false;
					}

					if (TypeRecognition::IsPrimitiveTypeFixArray(primitive_type))
					{
						if (!len)
						{
							//std::cout << "primitive_type length error:" << type_info.primitive_type << "\n";
							std::cout << fmt::format("The message {0} field {1} primitive_type {2} length not valid,must set value.", msg_name, field_name, primitive_type);
							return false;
						}

						if (len.value() <= 0)
						{
							std::cout << fmt::format("The message {0} field {1} primitive_type {2} length is {3},must >= 0", msg_name, field_name, primitive_type, len.value());
							return false;
						}
					}

					FieldInfoBase simple_info(FieldType::Primitive, field_name, primitive_type,len.value_or(0), description);
					msg_info.PushFiled(simple_info);
				}
				else if (v2.first == "Sequence")
				{
					if (msg_name_struct_map_.count(primitive_type) || type_info_map_.count(primitive_type)|| TypeRecognition::IsPrimitiveTypeValid(primitive_type))
					{

						if (TypeRecognition::IsPrimitiveTypeFixArray(primitive_type))
						{
							if (!len)
							{
								//std::cout << "primitive_type length error:" << type_info.primitive_type << "\n";
								std::cout << fmt::format("The message {0} field {1} primitive_type {2} length not valid,must set value.", msg_name, field_name, primitive_type);
								return false;
							}

							if (len.value() <= 0)
							{
								std::cout << fmt::format("The message {0} field {1} primitive_type {2} length is {3},must >= 0", msg_name, field_name, primitive_type, len.value());
								return false;
							}
						}

						FieldInfoBase struct_info(FieldType::Sequence, field_name, primitive_type,len.value_or(0), description);
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

		}

		//常量
		if (constants_tree)
		{
			for (auto& v1 : *constants_tree)
			{
				std::cout << v1.first << "," << v1.second.data() << "\n";

				std::string name = v1.second.get<std::string>("<xmlattr>.name");
				auto primitive_type = v1.second.get<std::string>("<xmlattr>.primitive_type");
				std::string description = v1.second.get<std::string>("<xmlattr>.description");
				auto len = v1.second.get_optional<int32_t>("<xmlattr>.length");

				if (TypeRecognition::IsPrimitiveTypeFixArray(primitive_type))
				{
					if (!len)
					{
						//std::cout << "primitive_type length error:" << type_info.primitive_type << "\n";
						std::cout << fmt::format("The Const {0} primitive_type {1} length not valid,must set value.", name, primitive_type);
						return false;
					}

					if (len.value() <= 0)
					{
						std::cout << fmt::format("The Const {0} primitive_type {1} length is {2},must >= 0",name, primitive_type, len.value());
						return false;
					}
				}

				ConstInfoBase const_info(name, primitive_type,len.value_or(0), description);
				for (auto& v2 : v1.second)
				{
					if (v2.first == "<xmlattr>")
						continue;

					std::cout << "message field:" << v2.first << "\n";
					std::string field_name = v2.second.get<std::string>("<xmlattr>.name");
					std::string description = v2.second.get<std::string>("<xmlattr>.description");
					std::string value = v2.second.get<std::string>("");

					FieldInfoValue field(value,field_name, primitive_type,len.value_or(0), description);

					const_info.PushFiled(field);
				}

				if (const_name_map_.count(name))
				{
					std::cout << fmt::format("const name  duplicate key,{} has been defined.\n", name);
					return false;
				}

				const_name_map_[name] = const_info;

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

		if (1)
		{

			std::cout << fmt::format("parse TEMPLATE_MESSAGE_H\n");
			inja::Template temp_msg_h = env.parse_template("TEMPLATE_MESSAGE_H.txt");
			std::cout << fmt::format("parse TEMPLATE_MESSAGE_CPP\n");
			inja::Template temp_msg_cpp = env.parse_template("TEMPLATE_MESSAGE_CPP.txt");

			for (auto& [key, value] : msg_name_struct_map_)
			{
				inja::json json;
				json["NAMESPACE"] = "HAO";
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
					auto type_info = type_info_map_[f.GetPrimitiveType()];
					j_field["F_TYPE_INFO"] =
					{
						{"T_NAME",type_info.GetName()},
						{"T_PRIMITIVE_TYPE",type_info.GetPrimitiveType()},
						{"T_LENGTH",type_info.GetLength()}
					};
					json["FIELDS"].push_back(j_field);
				}

				std::cout << fmt::format("write {}.h\n",key);
				env.write(temp_msg_h, json, key + ".h");
				std::cout << fmt::format("write {}.cpp\n", key);
				env.write(temp_msg_cpp, json, key + ".cpp");

			}
		}

		if (1)
		{
			std::cout << fmt::format("parse TEMPLATE_TYPES_H\n");
			inja::Template temp_types_h = env.parse_template("TEMPLATE_TYPES_H.txt");
			inja::json json_types;
			json_types["NAMESPACE"] = "HAO";
			json_types["TYPES"].push_back({ {"T_NAME","CHAR"},{"T_PRIMITIVE_TYPE","CHAR"},{"T_LENGTH",0},{"T_DESCRIPTION","CHAR"} });
			json_types["TYPES"].push_back({ {"T_NAME","UCHAR"},{"T_PRIMITIVE_TYPE","UCHAR"},{"T_LENGTH",0},{"T_DESCRIPTION","UNSIGNED CHAR"} });
			json_types["TYPES"].push_back({ {"T_NAME","BOOL"},{"T_PRIMITIVE_TYPE","BOOL"},{"T_LENGTH",0},{"T_DESCRIPTION","BOOL"} });
			json_types["TYPES"].push_back({ {"T_NAME","INT8"},{"T_PRIMITIVE_TYPE","INT8"},{"T_LENGTH",0},{"T_DESCRIPTION","INT8_T"} });
			json_types["TYPES"].push_back({ {"T_NAME","UINT8"},{"T_PRIMITIVE_TYPE","UINT8"},{"T_LENGTH",0},{"T_DESCRIPTION","UINT8_T"} });
			json_types["TYPES"].push_back({ {"T_NAME","INT16"},{"T_PRIMITIVE_TYPE","INT16"},{"T_LENGTH",0},{"T_DESCRIPTION","INT8_T"} });
			json_types["TYPES"].push_back({ {"T_NAME","UINT16"},{"T_PRIMITIVE_TYPE","UINT16"},{"T_LENGTH",0},{"T_DESCRIPTION","UINT16_T"} });
			json_types["TYPES"].push_back({ {"T_NAME","INT32"},{"T_PRIMITIVE_TYPE","INT32"},{"T_LENGTH",0},{"T_DESCRIPTION","INT32_T"} });
			json_types["TYPES"].push_back({ {"T_NAME","UINT32"},{"T_PRIMITIVE_TYPE","UINT32"},{"T_LENGTH",0},{"T_DESCRIPTION","UINT32_T"} });
			json_types["TYPES"].push_back({ {"T_NAME","INT64"},{"T_PRIMITIVE_TYPE","INT64"},{"T_LENGTH",0},{"T_DESCRIPTION","INT64_T"} });
			json_types["TYPES"].push_back({ {"T_NAME","UINT64"},{"T_PRIMITIVE_TYPE","UINT64"},{"T_LENGTH",0},{"T_DESCRIPTION","UINT64_T"} });
			json_types["TYPES"].push_back({ {"T_NAME","STRING"},{"T_PRIMITIVE_TYPE","STRING"},{"T_LENGTH",std::numeric_limits<uint32_t>::max()},{"T_DESCRIPTION","STD::STRING"} });

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

			std::cout << fmt::format("write Types.h\n");
			env.write(temp_types_h, json_types, "Types.h");
		}

		if (1)
		{
			std::cout << fmt::format("parse TEMPLATE_CONSTANTS_H\n");
			inja::Template temp_constants_h = env.parse_template("TEMPLATE_CONSTANTS_H.txt");
			std::cout << fmt::format("parse TEMPLATE_CONSTANTS_CPP\n");
			inja::Template temp_constants_cpp = env.parse_template("TEMPLATE_CONSTANTS_CPP.txt");
			inja::json json_constants;
			json_constants["NAMESPACE"] = "HAO";

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
					auto type_info = type_info_map_[f.GetPrimitiveType()];
					j_field["F_TYPE_INFO"] = { {"T_NAME",type_info.GetName()},{"T_PRIMITIVE_TYPE",type_info.GetPrimitiveType()},{"T_LENGTH",type_info.GetLength()} };
					json["FIELDS"].push_back(j_field);
				}
				std::cout << json << "\n";
				json_constants["CONSTANTS"].push_back(json);
			}

			std::cout << json_constants << "\n";

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
