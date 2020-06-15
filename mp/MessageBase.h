#pragma once
#include<stdint.h>
#include<string>
#include<ostream>
#include<sstream>
#include"ErrorCode.h"

namespace mp
{

	class Decoder;
	class Encoder;

	class MessageBase
	{
	public:
		MessageBase() {}
		virtual ~MessageBase() {}

		std::string ToString()
		{
			std::ostringstream ss;
			Dump(ss);
			return ss.str();
		}

		virtual uint32_t GetMsgType() = 0;

		virtual void FillDefaultValue() = 0;

		virtual mp::ErrorCode Decode(mp::Decoder& decoder) = 0;

		virtual mp::ErrorCode Encode(mp::Encoder& encoder) = 0;

		virtual void Dump(std::ostream& ostream) = 0;
	};

}