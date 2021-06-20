#pragma once
#include "DataBuffer.hpp"
#include <functional>

namespace mp
{
    namespace tmp
    {
        template< template<typename...> class U, typename Head >
        struct is_template_instant_of : std::false_type {};

        template< template <typename...> class U, typename... args >
        struct is_template_instant_of< U, U<args...> > : std::true_type {};

        template<typename Head>
        struct is_stdstring : is_template_instant_of<std::basic_string, Head >
        {};
        template<typename Head>
        struct is_stdstringview : is_template_instant_of<std::basic_string_view, Head >
        {};

        template<class Head>
        struct is_stdarray :std::is_array<Head> {};
        template<class Head, std::size_t N>
        struct is_stdarray<std::array<Head, N>> :std::true_type {};
        // optional:
        template<class Head>
        struct is_stdarray<Head const> :is_stdarray<Head> {};
        template<class Head>
        struct is_stdarray<Head volatile> :is_stdarray<Head> {};
        template<class Head>
        struct is_stdarray<Head volatile const> :is_stdarray<Head> {};



        template <typename F, typename... Args, std::size_t... Idx>
        constexpr auto TupleForeach(F&& f, const std::tuple<Args...>& t, std::index_sequence<Idx...>)
        {
            //auto v={ (std::forward<F>(f)(std::get<Idx>(t)),0) ... };    //参数包展开
            return (std::forward<F>(f)(std::get<Idx>(t)), ...);    //折叠表达式
        }

        template<typename Func, typename... Args>
        constexpr auto TupleForeach(Func&& f, std::tuple<Args...>&& tup)
        {
            return TupleForeach(std::forward<Func>(f), std::forward<std::tuple<Args...>>(tup), std::make_index_sequence < std::tuple_size<std::tuple<Args...>>{} > {});
        }

        template<std::size_t... Is>
        constexpr auto index_sequence_reverse(std::index_sequence<Is...> const&)
            -> decltype(std::index_sequence<sizeof...(Is) - 1U - Is...>{});

        template<std::size_t N>
        using make_index_sequence_reverse = decltype(index_sequence_reverse(std::make_index_sequence<N>{}));

        // iterate reverse order
        template<class F, class T>
        auto TupleForeachReverse(F&& f, T&& tup) 
        {
            const std::size_t size = std::tuple_size<std::decay_t<T>>::value;
            auto indexes = make_index_sequence_reverse<size>{};
            return TupleForeach(std::forward<F>(f), std::forward<T>(tup), indexes);
        }
     
        template<class F, class Head, class... Tail>
        bool FuncTupleArgs(F&& f, Head&& head, Tail&&... tail)
        {
            if constexpr (sizeof...(tail) > 0)
            {
                return FuncTupleArgs(std::forward<F>(f), std::forward<Tail>(tail)...) && f(std::forward<Head>(head));
            }
            else
            {
                return f(head);
            }
        }

        template<class F, typename... Ts>
        auto TupleForeachReverseWithReturnBool(F&& f, std::tuple<Ts...>&& tup)
        {
            return std::apply([&f](Ts const&... args){return  FuncTupleArgs(f,args...);},tup);
        }
    }

    template< typename Head, class... Tail>
    size_t GetBatchDataSize(Head&& value, Tail&&... tail)  noexcept
    {
        if constexpr (sizeof...(tail) > 0)
        {
            //return  (GetBatchDataSize(std::forward<Head>(value)) + ... + GetBatchDataSize(std::forward<Tail>(tail)));  //折叠表达式
            return GetBatchDataSize(std::forward<Head>(value)) + GetBatchDataSize(std::forward<Tail>(tail)...);   //参数包展开
        }
        else
        {
            using HeadType = std::remove_cvref_t<Head>;

            if constexpr (std::is_integral_v<HeadType>)
            {
                return sizeof(value);
            }
            else if constexpr (std::is_array<HeadType>::value)
            {
                return  std::extent<std::remove_cvref_t<Head>>::value;
            }
            else if constexpr (tmp::is_stdarray<HeadType>::value)
            {
                return  value.size();
            }
            else if constexpr (tmp::is_stdstring<HeadType>::value)
            {
                //static_assert(0, "Unsupported type std::string");
                return  4 + value.size();
            }
            else if constexpr (tmp::is_stdstringview<HeadType>::value)
            {
                //static_assert(0, "Unsupported type std::string_view");
                return  4 + value.size();
            }
            else
            {
                static_assert(0, "Unsupported type");
            }
        }
    }

    template<DataBuffer::ByteOrderEndian endian = DataBuffer::ByteOrderEndian::kNative, class Head, class... Tail>
    void WriteBatch(DataBuffer& buffer, Head&& value, Tail&&... tail)  noexcept
    {
        if constexpr (sizeof...(tail) > 0)
        {
            //return  { WriteBatch<endian>(buffer, std::forward<Head>(value)), WriteBatch<endian>(buffer, std::forward<Tail>(tail)...) }; //参数包展开
            return  (WriteBatch<endian>(buffer, std::forward<Head>(value)), ..., WriteBatch<endian>(buffer, std::forward<Tail>(tail)));//fold 折叠表达式
        }
        else
        {
            using HeadType = std::remove_cvref_t<Head>;
            if constexpr (std::is_integral_v<HeadType>)
            {
                return buffer.Write<endian>(value);
            }
            else if constexpr (tmp::is_stdstring<HeadType>::value)
            {
                return buffer.Write((int32_t)value.size()) && buffer.Write(value.data(), value.size());
            }
            else if constexpr (tmp::is_stdstringview<HeadType>::value)
            {
                return buffer.Write((int32_t)value.size()) && buffer.Write(value);
            }
            else if constexpr (tmp::is_stdarray<HeadType>::value)
            {
                return buffer.Write(value);

            }
            else
            {
                static_assert(0, "Unsupported type");
            }
        }
    }

    template<DataBuffer::ByteOrderEndian endian = DataBuffer::ByteOrderEndian::kNative, bool CheckSize = true, typename Head, class... Tail>
    bool WriteFrontBatch(DataBuffer& buffer, Head&& value, Tail&&... tail)  noexcept
    {
        if constexpr (CheckSize)
        {
            if (buffer.FrontWriteableBytes() < GetBatchDataSize(std::forward<Head>(value), std::forward<Tail>(tail)...))
            {
                return false;
            }
        }

        if constexpr (sizeof...(tail) > 0)
        {
            return WriteFrontBatch<endian, false>(buffer, std::forward<Tail>(tail)...) && WriteFrontBatch<endian, false>(buffer, std::forward<Head>(value));
        }
        else
        {
            using HeadType = std::remove_cvref_t<Head>;
            if constexpr (std::is_integral_v<HeadType>)
            {
                return buffer.WriteFront<endian>(value);
            }
            else if constexpr (tmp::is_stdstring<HeadType>::value)
            {
                return  buffer.WriteFront(value.data(), value.size()) && buffer.WriteFront((int32_t)value.size());
            }
            else if constexpr (tmp::is_stdstringview<HeadType>::value)
            {
                return  buffer.WriteFront(value) && buffer.WriteFront((int32_t)value.size());
            }
            else if constexpr (tmp::is_stdarray<HeadType>::value)
            {
                return buffer.WriteFront(value);

            }
            else
            {
                static_assert(0, "Unsupported type");
            }
        }

        return false;
    }



}