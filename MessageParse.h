#pragma once

#include<iostream>
#include<string>
#include<algorithm>
#include<vector>
#include<unordered_map>
#include<map>

#include "fmt/format.h"

enum class EndianType :uint8_t
{
    native = 0,
    big = 1,
    little = 2
};

enum class FieldType :uint8_t
{
    Primitive,
    Sequence
};


class TypeInfoBase
{
public:
    TypeInfoBase() {}
    TypeInfoBase(const std::string& name, const std::string& primitive_type, int32_t length, const std::string& description)
    {
        name_ = name;
        primitive_type_ = primitive_type;
        length_ = length;
        description_ = description;
    }

    const std::string& GetName()
    {
        return name_;
    }

    const std::string& GetPrimitiveType()
    {
        return primitive_type_;
    }

    const std::string& GetDescription()
    {
        return description_;
    }

    uint32_t GetLength()
    {
        return length_;
    }
private:
    std::string name_;
    std::string primitive_type_;
    int32_t  length_ = 0;
    std::string description_;
};

class FieldInfoBase :public TypeInfoBase
{
public:
    FieldInfoBase(FieldType field_type, const std::string& name, const std::string& primitive_type, int32_t length, const std::string& description)
        :field_type_(field_type), TypeInfoBase(name, primitive_type, length, description)
    {

    }

    FieldType GetFiledType()
    {
        return field_type_;
    }

private:
    FieldType field_type_;
};

class MessageInfoBase
{
public:
    MessageInfoBase() {}
    MessageInfoBase(const std::string& name, int32_t pktno, const std::string& inherit, const std::string& description)
    {
        name_ = name;
        pktno_ = pktno;
        inherit_ = inherit;
        description_ = description;
    }

    const std::string& GetName()
    {
        return name_;
    }

    int32_t GetPktNo()
    {
        return pktno_;
    }

    const std::string& GetInherit()
    {
        return inherit_;
    }

    const std::string& GetDescription()
    {
        return description_;
    }

    void PushFiled(const FieldInfoBase& field)
    {
        v_field_.push_back(field);
    }

    bool ExistField(const std::string& name)
    {
        auto it = std::find_if(v_field_.begin(), v_field_.end(),
            [&](FieldInfoBase& field_info)
            {
                return  (field_info.GetName() == name);
            });

        return (it != v_field_.end());
    }

    const std::vector<FieldInfoBase>& GetFields()
    {
        return v_field_;
    }
private:
    std::string name_;
    int32_t pktno_;
    std::string inherit_;
    std::string description_;
    std::vector<FieldInfoBase> v_field_;
};

class FieldInfoValue :public TypeInfoBase
{
public:
    FieldInfoValue(const std::string& value, const std::string& name, const std::string& primitive_type, int32_t length, const std::string& description)
        :value_(value), TypeInfoBase(name, primitive_type, length, description)
    {

    }

    std::string GetValue()
    {
        return value_;
    }

private:
    std::string value_;
};

class ConstInfoBase :public TypeInfoBase
{
public:
    ConstInfoBase() {}
    ConstInfoBase(const std::string& name, const std::string& primitive_type, int32_t length, const std::string& description)
        :TypeInfoBase(name, primitive_type, length, description)
    {
    }

    void PushFiled(const FieldInfoValue& field)
    {
        v_field.push_back(field);
    }

    bool ExistField(const std::string& name)
    {
        auto it = std::find_if(v_field.begin(), v_field.end(),
            [&](FieldInfoValue& field_info)
            {
                return  (field_info.GetName() == name);
            });

        return (it != v_field.end());
    }

    const std::vector<FieldInfoValue>& GetFields()
    {
        return v_field;
    }
private:
    std::vector<FieldInfoValue> v_field;
};

namespace TypeRecognition
{
    static std::unordered_map<std::string, int32_t> PrimitiveTypeMap =
    {
        {"CHAR",1},
        {"UCHAR",1},
        {"BOOL",1},
        {"INT8",1},
        {"UINT8",1},
        {"INT16",2},
        {"UINT16",2},
        {"INT32",4},
        {"UINT32",4},
        {"INT64",8},
        {"UINT64",8},
        {"FIXARRAY",std::numeric_limits<int32_t>::min()},
        {"STRING",std::numeric_limits<int32_t>::max()}
    };

    static bool IsPrimitiveTypeValid(const std::string& type)
    {
        return PrimitiveTypeMap.count(type);
    }

    static bool IsPrimitiveTypeInt(const std::string& type)
    {
        return  type.compare("STRING") != 0 && type.compare("FIXARRAY") != 0;
    }

    static int32_t GetPrimitiveTypeIntSize(const std::string& type)
    {
        return  PrimitiveTypeMap[type];
    }

    static bool IsPrimitiveTypeString(const std::string& type)
    {
        return type.compare("STRING") == 0;
    }

    static bool IsPrimitiveTypeFixArray(const std::string& type)
    {
        return type.compare("FIXARRAY") == 0;
    }
};


class MessageParser
{
public:
    MessageParser()
    {
        type_info_map_ =
        {
            //基本类型
            {"CHAR",{"CHAR","CHAR",1,""}},
            {"UCHAR",{"UCHAR","UCHAR",1,""}},
            {"BOOL",{"BOOL","BOOL",1,""}},
            {"INT8",{"INT8","INT8",1,""}},
            {"UINT8",{"UINT8","UINT8",1,""}},
            {"INT16",{"INT16","INT16",2,""}},
            {"UINT16",{"UINT16","UINT16",2,""}},
            {"INT32",{"INT32","INT32",4,""}},
            {"UINT32",{"UINT32","UINT32",4,""}},
            {"INT64",{"INT64","INT64",8,""}},
            {"UINT64",{"UINT64","UINT64",8,""}},
            {"STRING",{"STRING","STRING",std::numeric_limits<int32_t>::max(),""}}
        };
    }
    virtual ~MessageParser() {}

    bool LoadXml(const std::string& file_path);

    bool Write(const std::string& template_path, const std::string& write_path);

private:
    std::string file_name_;
    std::vector<std::string> v_namespace_;
    EndianType endian_;
    //保存类型信息
    std::unordered_map<std::string, TypeInfoBase> type_info_map_;
    std::vector<TypeInfoBase> v_type_info_;
    //保存消息信息
    std::unordered_map<std::string, MessageInfoBase> msg_name_struct_map_;
    std::unordered_map<uint32_t, MessageInfoBase> msg_pktno_struct_map_;
    std::vector<MessageInfoBase> v_msg_struct_info_;
    //保存常量信息
    std::unordered_map<std::string, ConstInfoBase> const_name_map_;
    std::vector<ConstInfoBase> v_const_info_;

};