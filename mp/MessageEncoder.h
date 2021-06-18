#pragma once
#include<exception>
#include<iostream>

#include"MpTypes.h"
#include"DataBuffer.hpp"

namespace mp
{
    class MessageEncoder
    {
    public:
        MessageEncoder(DataBuffer& data_buffer, bool host_to_network_byte_order = true) : data_buffer_(data_buffer),
            host_to_network_byte_order_(host_to_network_byte_order)
        {

        }

        template<typename T, typename std::enable_if <std::is_integral<T>::value, int >::type = 0 >
        ErrorCode Write(T value)
        {

            if (host_to_network_byte_order_)
            {
                data_buffer_.Write<DataBuffer::kBig>(value);
            }
            else
            {
                data_buffer_.Write(value);
            }

            return ErrorCode::kSuccess;
        }

        template<uint32_t N>
        ErrorCode Write(const std::array<char, N>& value)
        {
            return Write(value.data(), N);
        }

        ErrorCode Write(const std::string& value)
        {
            return Write(value.data(), (uint32_t)value.size());
        }

        ErrorCode Write(const char* p, uint32_t size)
        {

            data_buffer_.Write(p, size);

            return ErrorCode::kSuccess;
        }
    private:
        DataBuffer& data_buffer_;
        bool host_to_network_byte_order_;
    };
}