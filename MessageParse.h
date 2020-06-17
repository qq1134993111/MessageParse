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
	TypeInfoBase(const std::string& name, const std::string& primitive_type, uint32_t length, const std::string& description)
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
	int32_t  length_=0;
	std::string description_;
};

class FieldInfoBase
{
public:
	FieldInfoBase(FieldType filed_type, const std::string& name, const std::string& primitive_type, const std::string& description)
	{
		filed_type_ = filed_type;
		name_ = name;
		primitive_type_ = primitive_type;
		description_ = description;
	}

	FieldType GetFiledType()
	{
		return filed_type_;
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

private:
	FieldType filed_type_;
	std::string name_;
	std::string primitive_type_;
	std::string description_;
};

class MessageInfoBase
{
public:
	MessageInfoBase() {}
	MessageInfoBase(const std::string& name, uint32_t pktno, const std::string& inherit, const std::string& description)
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

	uint32_t GetPktNo()
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
	uint32_t pktno_;
	std::string inherit_;
	std::string description_;
	std::vector<FieldInfoBase> v_field_;
};

class FieldInfoValue
{
public:
	FieldInfoValue(const std::string& name, const std::string& primitive_type, const std::string& value, const std::string& description)
	{

		name_ = name;
		primitive_type_ = primitive_type;
		value_ = value;
		description_ = description;
	}

	std::string GetValue()
	{
		return value_;
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

private:
	std::string name_;
	std::string primitive_type_;
	std::string value_;
	std::string description_;
};

class ConstInfoBase
{
public:
	ConstInfoBase() {}
	ConstInfoBase(const std::string& name, const std::string& primitive_type, const std::string& description)
	{
		name_ = name;
		primitive_type_ = primitive_type;
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
	std::string name_;
	std::string primitive_type_;
	std::string description_;
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
		{"FIXARRAY",-1},
		{"STRING",-1}
	};

	static bool IsPrimitiveTypeValid(const std::string& type)
	{
		return PrimitiveTypeMap.count(type);
	}

	static bool IsPrimitiveTypeInt(const std::string& type)
	{
		return  type.compare("STRING") != 0 && type.compare("FIXARRAY") != 0;
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
			{"CHAR",{"CHAR","CHAR",0,""}},
			{"UCHAR",{"UCHAR","UCHAR",0,""}},
			{"BOOL",{"BOOL","BOOL",0,""}},
			{"INT8",{"INT8","INT8",0,""}},
			{"UINT8",{"UINT8","UINT8",0,""}},
			{"INT16",{"INT16","INT16",0,""}},
			{"UINT16",{"UINT16","UINT16",0,""}},
			{"INT32",{"INT32","INT32",0,""}},
			{"INT64",{"INT64","INT64",0,""}},
			{"UINT64",{"UINT64","UINT64",0,""}},
			{"STRING",{"STRING","STRING",std::numeric_limits<uint32_t>::max(),""}}
		};
	}
	virtual ~MessageParser() {}

	bool LoadXml(const std::string& file_path);

	bool Write(const std::string& template_path,const std::string& write_path);

private:
	EndianType endian_;
	std::unordered_map<std::string, TypeInfoBase> type_info_map_;
	std::unordered_map<std::string, MessageInfoBase> msg_name_struct_map_;
	std::unordered_map<uint32_t, MessageInfoBase> msg_pktno_struct_map_;
	std::unordered_map<std::string, ConstInfoBase> const_name_map_;
	
};