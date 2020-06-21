#pragma once
#include<stdint.h>
namespace mp
{
	enum  class ErrorCode :int32_t
	{
		kSuccess = 0,  //³É¹¦
		kWriteError,
		kReadError
	};
}