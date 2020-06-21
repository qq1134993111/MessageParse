#pragma once

#include <string>
#include <array>
#include "boost/array.hpp"
#include "boost/algorithm/string/trim.hpp"

namespace mp
{

    namespace notstd
    {

        template<class T>
        struct is_array :std::is_array<T> {};

        template<class T, std::size_t N>
        struct is_array<std::array<T, N>> :std::true_type {};

        template<class T, std::size_t N>
        struct is_array<boost::array<T, N>> :std::true_type {};

        template<class T>
        struct std_array {};

        template<class T, std::size_t N>
        struct std_array<std::array<T, N>> :std::integral_constant<std::size_t, N>
        {
            typedef T type;
        };

        template<class T, std::size_t N>
        struct std_array<boost::array<T, N>> :std::integral_constant<std::size_t, N>
        {
            typedef T type;
        };

    }

    //char[] -> string
    template<typename T, typename std::enable_if<
        std::is_array<T>::value&&
        std::is_same<char, typename std::remove_extent<T>::type>::value,
        int >::type = __LINE__ >
        void SetValue(std::string& str_value, const T& t, bool trim_right = true)
    {
        static_assert(std::is_array<T>::value && std::is_same<char, typename std::remove_extent<T>::type>::value, "type must be char array");
        str_value.assign(t, std::extent<T>::value);
        if (trim_right)
        {
            boost::algorithm::trim_right(str_value);

        }
    }

    //string ->char[]
    template<typename T, typename std::enable_if<
        std::is_array<T>::value&&
        std::is_same<char, typename std::remove_extent<T>::type>::value,
        int>::type = __LINE__>
        void SetValue(T& t, const std::string& str_value, bool fill_space = true)
    {
        static_assert(std::is_array<T>::value && std::is_same<char, typename std::remove_extent<T>::type>::value, "type must be char array");

        auto min_len = std::min(std::extent<T>::value, str_value.size());
        memcpy((typename std::decay<T>::type)t, str_value.c_str(), min_len);
        if (fill_space && min_len < std::extent<T>::value)
        {
            memset((typename std::decay<T>::type)t + min_len, ' ', std::extent<T>::value - min_len);
        }
    }


    //string -> string 

    inline void SetValue(std::string& str1, const std::string& str2, bool trim_right = true)
    {
        str1.assign(str2);
        if (trim_right)
        {
            boost::algorithm::trim_right(str1);
        }
    }

    //char[] -> char[]
    template<typename T1, typename T2, typename std::enable_if<
        std::is_array<T1>::value&&
        std::is_same<char, typename std::remove_extent<T1>::type>::value&&
        std::is_array<T2>::value&&
        std::is_same<char, typename std::remove_extent<T2>::type>::value,
        int>::type = __LINE__>
        void	SetValue(T1& t1, const T2& t2, bool fill_space = true)
    {
        static_assert(std::is_array<T1>::value && std::is_same<char, typename std::remove_extent<T1>::type>::value, "type must be char array");
        static_assert(std::is_array<T2>::value && std::is_same<char, typename std::remove_extent<T2>::type>::value, "type must be char array");


        auto min_len = std::min(std::extent<T1>::value, std::extent<T2>::value);
        memcpy((typename std::decay<T1>::type)t1, (typename std::decay<T2>::type)t2, min_len);

        if (fill_space && min_len < std::extent<T1>::value)
        {
            memset((typename std::decay<T1>::type)t1 + min_len, ' ', std::extent<T1>::value - min_len);
        }
    }

    //array<char> -> char[]
    template<typename T1, typename T2, typename std::enable_if<
        std::is_array<T1>::value&&
        std::is_same<char, typename std::remove_extent<T1>::type>::value&&
        notstd::is_array<T2>::value&&
        std::is_same<char, typename notstd::std_array<T2>::type>::value,
        int>::type = __LINE__>
        void SetValue(T1& t1, const T2& t2, bool fill_space = true)
    {
        static_assert(std::is_array<T1>::value && std::is_same<char, typename std::remove_extent<T1>::type>::value, "type must be char array");
        static_assert(notstd::is_array<T2>::value && std::is_same<char, typename notstd::std_array<T2>::type >::value, "type must be char array");

        auto min_len = std::min(std::extent<T1>::value, notstd::std_array<T2>::value);
        memcpy((typename std::decay<T1>::type)t1, (typename notstd::std_array<T2>::type*)t2.data(), min_len);
        if (fill_space && min_len < std::extent<T1>::value)
        {
            memset((typename std::decay<T1>::type)t1 + min_len, ' ', std::extent<T1>::value - min_len);
        }
    }

    //char[] -> array<char>
    template<typename T1, typename T2, typename std::enable_if<
        notstd::is_array<T1>::value&&
        std::is_same<char, typename notstd::std_array<T1>::type>::value&&
        std::is_array<T2>::value&&
        std::is_same<char, typename std::remove_extent<T2>::type>::value,
        int>::type = __LINE__ >
        void SetValue(T1& t1, const T2& t2, bool fill_space = true)
    {
        static_assert(notstd::is_array<T1>::value && std::is_same<char, typename notstd::std_array<T1>::type >::value, "type must be char array");
        static_assert(std::is_array<T2>::value && std::is_same<char, typename std::remove_extent<T2>::type>::value, "type must be char array");


        auto min_len = std::min(std::extent<T2>::value, notstd::std_array<T1>::value);
        memcpy((typename notstd::std_array<T1>::type*)t1.data(), (typename std::decay<T2>::type)t2, min_len);
        if (fill_space && min_len < notstd::std_array<T1>::value)
        {
            memset((typename  notstd::std_array<T1>::type*)t1.data() + min_len, ' ', notstd::std_array<T1>::value - min_len);
        }
    }

    //string -> array<char>
    template<typename T, typename std::enable_if<
        notstd::is_array<T>::value&&
        std::is_same<char, typename notstd::std_array<T>::type>::value,
        int>::type = __LINE__>
        void SetValue(T& t, const std::string& str_value, bool fill_space = true)
    {
        static_assert(notstd::is_array<T>::value && std::is_same<char, typename notstd::std_array<T>::type >::value, "type must be char array");

        auto min_len = std::min(notstd::std_array<T>::value, str_value.size());
        memcpy((typename notstd::std_array<T>::type*)t.data(), str_value.c_str(), min_len);
        if (fill_space && min_len < notstd::std_array<T>::value)
        {
            memset((typename  notstd::std_array<T>::type*)t.data() + min_len, ' ', notstd::std_array<T>::value - min_len);
        }
    }



    //array<char> ->string
    template<typename T, typename std::enable_if<
        notstd::is_array<T>::value&&
        std::is_same<char, typename notstd::std_array<T>::type>::value,
        int>::type = __LINE__>
        void SetValue(std::string& str_value, const T& t, bool trim_right = true)
    {
        static_assert(notstd::is_array<T>::value && std::is_same<char, typename notstd::std_array<T>::type >::value, "type must be char array");

        str_value.assign((typename notstd::std_array<T>::type*)t.data(), notstd::std_array<T>::value);
        if (trim_right)
        {
            boost::algorithm::trim_right(str_value);

        }
    }

    //array<char> -> array<char>
    template<typename T1, typename T2, typename std::enable_if<
        notstd::is_array<T1>::value&&
        std::is_same<char, typename notstd::std_array<T1>::type>::value&&
        notstd::is_array<T2>::value&&
        std::is_same<char, typename notstd::std_array<T2>::type>::value,
        int>::type = __LINE__>
        void SetValue(T1& t1, const T2& t2, bool fill_space = true)
    {
        static_assert(notstd::is_array<T1>::value && std::is_same<char, typename notstd::std_array<T1>::type >::value, "type must be char array");
        static_assert(notstd::is_array<T2>::value && std::is_same<char, typename notstd::std_array<T2>::type >::value, "type must be char array");

        auto min_len = std::min(notstd::std_array<T1>::value, notstd::std_array<T2>::value);
        memcpy((typename notstd::std_array<T1>::type*)t1.data(), (typename notstd::std_array<T2>::type*)t2.data(), min_len);

        if (fill_space && min_len < notstd::std_array<T1>::value)
        {
            memset((typename  notstd::std_array<T1>::type*)t1.data() + min_len, ' ', notstd::std_array<T1>::value - min_len);
        }
    }

    //arithmetic
    template<typename T1, typename T2, typename std::enable_if<
        std::is_arithmetic<T1>::value&&
        std::is_arithmetic<T2>::value&&
        std::is_convertible<T2, T1>::value,
        int>::type = __LINE__>
        void SetValue(T1& t1, const T2& t2)
    {
        t1 = t2;
    }

}