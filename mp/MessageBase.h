#pragma once
#include<stdint.h>
#include<string>
#include<ostream>
#include<sstream>
#include"MpTypes.h"

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

        virtual void FillDefaultValue() = 0;

        virtual MsgType_Def GetMsgType() = 0;

        virtual uint32_t GetMsgSize() = 0;

        virtual mp::ErrorCode Decode(mp::MessageDecoder& decoder) = 0;

        virtual mp::ErrorCode Encode(mp::MessageEncoder& encoder) = 0;

        virtual void Dump(std::ostream& ostream) = 0;


    };

}