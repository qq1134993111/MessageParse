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
            try
            {
                if (host_to_network_byte_order_)
                {
                    data_buffer_.ReadIntegerBE(value);
                }
                else
                {
                    data_buffer_.Read(value);
                }
            }
            catch (std::exception& e)
            {
                std::cout << "MessageDecoder Read exception : " << e.what() << "\n";
                return ErrorCode::kReadError;
            }
            catch (...)
            {
                std::cout << "MessageDecoder Read unkown exception \n";
                return ErrorCode::kReadError;
            }

            return ErrorCode::kSuccess;
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
            try
            {
                data_buffer_.Read(p, size);
            }
            catch (std::exception& e)
            {
                std::cout << "MessageDecoder Read exception : " << e.what() << "\n";
                return ErrorCode::kReadError;
            }
            catch (...)
            {
                std::cout << "MessageDecoder Read unkown exception \n";
                return ErrorCode::kReadError;
            }
            return ErrorCode::kSuccess;
        }

    private:
        DataBuffer& data_buffer_;
        bool host_to_network_byte_order_;
    };
}