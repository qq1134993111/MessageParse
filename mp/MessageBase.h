#pragma once
#include<stdint.h>
#include<string>
#include<ostream>
#include<sstream>
#include"ErrorCode.h"

namespace mp
{

	class MessageDecoder;
	class MessageEncoder;

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

		virtual mp::ErrorCode Decode(mp::MessageDecoder& decoder) = 0;

		virtual mp::ErrorCode Encode(mp::MessageEncoder& encoder) = 0;

		virtual void Dump(std::ostream& ostream) = 0;
	};

}