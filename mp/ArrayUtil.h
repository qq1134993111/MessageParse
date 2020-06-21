#pragma once

#include<string>
#include<array>
#include"boost/algorithm/string.hpp"

namespace mp
{
    template<std::size_t N>
    std::string ToString(const std::array<char, N>& char_array)
    {
        std::string str_data;
        str_data.assign(char_array.data(), char_array.size());
        return str_data;
    }

    template<std::size_t N>
    std::string ToStringTrim(const std::array<char, N>& char_array)
    {
        return boost::trim_copy(std::string(char_array.data(), char_array.size()));
    }

    template<std::size_t N>
    std::string ToStringTrimRigth(const std::array<char, N>& char_array)
    {
        return boost::trim_right_copy(std::string(char_array.data(), char_array.size()));
    }

    template<std::size_t N>
    std::string ToStringTrimLeft(const std::array<char, N>& char_array)
    {
        return boost::trim_left_copy(std::string(char_array.data(), char_array.size()));
    }

    template<std::size_t N>
    void CopyToArray(std::array<char, N>& char_array, const std::string& str_data)
    {
        char_array.fill(' ');
        memcpy(char_array.data(), str_data.data(), std::min(N, str_data.size()));
    }

    template<std::size_t N>
    std::array<char, N> ToArray(const std::string& str_data)
    {
        std::array<char, N> char_array;
        CopyToArray(char_array, str_data);
        return char_array;
    }
}