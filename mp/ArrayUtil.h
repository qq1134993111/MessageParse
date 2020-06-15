#pragma once

#include<string>
#include<array>
#include"boost/algorithm/string.hpp"
namespace mp
{
	template<std::size_t N>
	std::string ToStringTrim(std::array<char, N> char_array)
	{
		std::string str_data(char_array.data(), char_array.size());
		return boost::trim_copy(str_data);
	}

	template<std::size_t N>
	std::array<char, N> ToArray(const std::string& str_data)
	{
		std::array<char, N> char_array;
		char_array.fill(' ');
		memcpy(char_array.data(), str_data.data(), std::min(N, str_data.size()));
		return char_array;
	}
}