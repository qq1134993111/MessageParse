#pragma once
#include<stdint.h>
namespace mp
{
    using MsgType_Def = uint32_t;

    enum  class ErrorCode :int32_t
    {
        kSuccess = 0,  //³É¹¦
        kWriteError,
        kReadError
    };
}