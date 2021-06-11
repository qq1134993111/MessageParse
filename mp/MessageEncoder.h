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
            try
            {
                if (host_to_network_byte_order_)
                {
                    data_buffer_.WriteIntegerBE(value);
                }
                else
                {
                    data_buffer_.Write(value);
                }
            }
            catch (std::exception& e)
            {
                std::cout << "MessageDecoder Write exception : " << e.what() << "\n";
                return ErrorCode::kWritError;
            }
            catch (...)
            {
                std::cout << "MessageDecoder Write unkown exception \n";
                return ErrorCode::kWritError;
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
            try
            {
                data_buffer_.Write(p, size);
            }
            catch (std::exception& e)
            {
                std::cout << "MessageDecoder Write exception : " << e.what() << "\n";
                return ErrorCode::kWritError;
            }
            catch (...)
            {
                std::cout << "MessageDecoder Write unkown exception \n";
                return ErrorCode::kWritError;
            }
            return ErrorCode::kSuccess;
        }
    private:
        DataBuffer& data_buffer_;
        bool host_to_network_byte_order_;
    };
}