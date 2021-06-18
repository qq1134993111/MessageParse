#pragma once
#include<exception>
#include<iostream>

#include"MpTypes.h"
#include"DataBuffer.hpp"

namespace mp
{
    class MessageDecoder
    {
    public:
        MessageDecoder(DataBuffer& data_buffer, bool host_to_network_byte_order = true) : data_buffer_(data_buffer),
            host_to_network_byte_order_(host_to_network_byte_order)
        {

        }

        template<typename T, typename std::enable_if <std::is_integral<T>::value, int >::type = 0 >
        ErrorCode Read(T& value)
        {
            if (host_to_network_byte_order_)
            {
                return data_buffer_.Read<DataBuffer::kBig>(value) ? ErrorCode::kSuccess : ErrorCode::kReadError;
            }
            else
            {
                return data_buffer_.Read(value) ? ErrorCode::kSuccess : ErrorCode::kReadError;
            }
        }

        template<std::size_t N>
        ErrorCode Read(std::array<char, N>& value)
        {
            return Read(value.data(), N);
        }

        ErrorCode Read(std::string& value)
        {
            return Read(value.data(), (uint32_t)value.size());
        }

        ErrorCode Read(char* p, uint32_t size)
        {

            return data_buffer_.Read(p, size) ? ErrorCode::kSuccess : ErrorCode::kReadError;
        }

    private:
        DataBuffer& data_buffer_;
        bool host_to_network_byte_order_;
    };
}