#pragma once
#include"ErrorCode.h"
namespace mp
{
	class MessageEncoder
	{
	public:
		template<typename T>
		ErrorCode Write(const T& value)
		{
			return ErrorCode::kSuccess;
		}

		ErrorCode Write(const char* p,size_t size)
		{
			return ErrorCode::kSuccess;
		}
	};
}