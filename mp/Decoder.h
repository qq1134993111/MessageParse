#pragma once
#include"ErrorCode.h"
namespace mp
{
	class Decoder
	{
	public:
		template<typename T>
		ErrorCode Read(T& value)
		{
			return ErrorCode::kSuccess;
		}

		ErrorCode Read(char* p,size_t size)
		{
			return ErrorCode::kSuccess;
		}
	};
}