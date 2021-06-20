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
            return std::apply([&f](Ts const&... args) {return  FuncTupleArgs(std::forward<F>(f), args...); }, tup);
        }
    }

    template< typename Head, class... Tail>
    size_t GetBatchWriteDataSize(Head&& value, Tail&&... tail)  noexcept
    {
        if constexpr (sizeof...(tail) > 0)
        {
            //return  (GetBatchDataSize(std::forward<Head>(value)) + ... + GetBatchDataSize(std::forward<Tail>(tail)));  //折叠表达式
            return GetBatchWriteDataSize(std::forward<Head>(value)) + GetBatchWriteDataSize(std::forward<Tail>(tail)...);   //参数包展开
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
    void BatchWrite(DataBuffer& buffer, Head&& value, Tail&&... tail)  noexcept
    {
        if constexpr (sizeof...(tail) > 0)
        {
            //return  { WriteBatch<endian>(buffer, std::forward<Head>(value)), WriteBatch<endian>(buffer, std::forward<Tail>(tail)...) }; //参数包展开
            return  (BatchWrite<endian>(buffer, std::forward<Head>(value)), ..., BatchWrite<endian>(buffer, std::forward<Tail>(tail)));//fold 折叠表达式
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
    bool BatchWriteFront(DataBuffer& buffer, Head&& value, Tail&&... tail)  noexcept
    {
        if constexpr (CheckSize)
        {
            if (buffer.FrontWriteableBytes() < GetBatchWriteDataSize(std::forward<Head>(value), std::forward<Tail>(tail)...))
            {
                return false;
            }
        }

        if constexpr (sizeof...(tail) > 0)
        {
            return BatchWriteFront<endian, false>(buffer, std::forward<Tail>(tail)...) && BatchWriteFront<endian, false>(buffer, std::forward<Head>(value));
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


    template<DataBuffer::ByteOrderEndian endian = DataBuffer::ByteOrderEndian::kNative, typename Head, class... Tail>
    bool BatchReadImpl(DataBuffer& buffer, Head& value, Tail&... tail)  noexcept
    {
        if constexpr (sizeof...(tail) > 0)
        {
            return BatchReadImpl<endian>(buffer, value) && BatchReadImpl<endian>(buffer, tail...);
        }
        else
        {
            using HeadType = std::remove_cvref_t<Head>;
            if constexpr (std::is_integral_v<HeadType>)
            {
                return buffer.Read<endian>(value);
            }
            else if constexpr (tmp::is_stdstring<HeadType>::value)
            {
                int32_t prefix_size = 0;
                if (!buffer.Read<endian>(prefix_size))
                {
                    return false;
                }

                if (buffer.Size() < prefix_size)
                {
                    return false;
                }

                try
                {
                    value.resize(prefix_size);
                }
                catch (...)
                {
                    return false;
                }

                return buffer.Read(value.data(), value.size());
            }
            else if constexpr (tmp::is_stdstringview<HeadType>::value)
            {
                int32_t prefix_size = 0;
                if (!buffer.Read<endian>(prefix_size))
                {
                    return false;
                }

                if (prefix_size != value.size())
                {
                    return false;
                }

                if (buffer.Size() < prefix_size)
                {
                    return false;
                }

                return buffer.Read(value.data(), value.size());

            }
            else if constexpr (tmp::is_stdarray<HeadType>::value)
            {
                return buffer.Read(value);

            }
            else
            {
                static_assert(0, "Unsupported type");
            }
        }

    }

    template<DataBuffer::ByteOrderEndian endian = DataBuffer::ByteOrderEndian::kNative, class... ARGS>
    bool BatchRead(DataBuffer& buffer, ARGS&&... args)  noexcept
    {
        auto ptr = buffer.Data();
        if (!BatchReadImpl<endian>(buffer, std::forward<ARGS>(args)...))
        {
            buffer.UnConsume(buffer.Data() - ptr);
            return false;
        }

        return true;
    }

    template<DataBuffer::ByteOrderEndian endian = DataBuffer::ByteOrderEndian::kNative, typename Head, class... Tail>
    bool GetBatchReadDataSizeImpl(DataBuffer& buffer, size_t& already_read_size, Head&& value, Tail&&... tail)  noexcept
    {
        if constexpr (sizeof...(tail))
        {
            return GetBatchReadDataSizeImpl<endian>(buffer, already_read_size, value) && GetBatchReadDataSizeImpl<endian>(buffer, already_read_size, tail...);
        }
        else
        {
            using HeadType = std::remove_cvref_t<Head>;
            if constexpr (std::is_integral_v<HeadType>)
            {
                constexpr size_t n = sizeof(value);
                if (buffer.Size() >= n)
                {
                    already_read_size += n;
                    buffer.Consume(n);
                    return true;
                }

                return false;
            }
            else if constexpr (tmp::is_stdstring<HeadType>::value)
            {
                int32_t prefix_size = 0;
                if (!buffer.Read<endian>(prefix_size))
                {
                    return false;
                }

                already_read_size += sizeof(prefix_size);

                if (buffer.Size() < prefix_size)
                {
                    return false;
                }

                already_read_size += prefix_size;

                buffer.Consume(prefix_size);
                return true;
            }
            else if constexpr (tmp::is_stdstringview<HeadType>::value)
            {
                int32_t prefix_size = 0;
                if (!buffer.Read(prefix_size))
                {
                    return false;
                }

                already_read_size += sizeof(prefix_size);

                if (prefix_size != value.size())
                {
                    return false;
                }

                already_read_size += prefix_size;

                buffer.Consume(prefix_size);

                return true;

            }
            else if constexpr (std::is_array<HeadType>::value)
            {
                constexpr size_t n = std::extent<std::remove_cvref_t<Head>>::value;
                if (buffer.Size() >= n)
                {
                    buffer.Consume(n);
                    already_read_size += n;
                    return true;
                }

                return false;
            }
            else if constexpr (tmp::is_stdarray<HeadType>::value)
            {
                constexpr size_t n = std::tuple_size<HeadType>{};
                if (buffer.Size() >= n)
                {
                    buffer.Consume(n);
                    already_read_size += n;
                    return true;
                }

                return false;
            }
            else
            {
                static_assert(0, "Unsupported type");
            }
        }
    }

    template<DataBuffer::ByteOrderEndian endian = DataBuffer::ByteOrderEndian::kNative, class... ARGS>
    size_t GetBatchReadDataSize(DataBuffer& buffer, ARGS&&... args)  noexcept
    {
        auto ptr = buffer.Data();
        size_t size = 0;
        if (!GetBatchReadDataSizeImpl(buffer, size, std::forward<ARGS>(args)...))
        {
            assert(buffer.Data() - ptr == size);
            buffer.UnConsume(size);
            return 0;
        }

        assert(buffer.Data() - ptr == size);
        buffer.UnConsume(size);

        return size;
    }

    template<DataBuffer::ByteOrderEndian endian = DataBuffer::ByteOrderEndian::kNative, class... Types>
    struct BatchReadDataSize
    {
        bool operator()(DataBuffer& buffer, size_t& already_read_size) const
        {
            return true;
        }
    };

    template<DataBuffer::ByteOrderEndian endian, class Type>
    struct BatchReadDataSize<endian, Type>
    {
        bool operator()(DataBuffer& buffer, size_t& already_read_size) const
        {
            using RealType = std::remove_cvref_t<Type>;
            if constexpr (std::is_integral_v<RealType>)
            {
                constexpr size_t n = sizeof(RealType);
                if (buffer.Size() >= n)
                {
                    already_read_size += n;
                    buffer.Consume(n);
                    return true;
                }

                return false;
            }
            else if constexpr (tmp::is_stdstring<RealType>::value)
            {
                int32_t prefix_size = 0;
                if (!buffer.Read<endian>(prefix_size))
                {
                    return false;
                }

                already_read_size += sizeof(prefix_size);

                if (buffer.Size() < prefix_size)
                {
                    return false;
                }

                already_read_size += prefix_size;

                buffer.Consume(prefix_size);
                return true;
            }
            /*else if constexpr (tmp::is_stdstringview<RealType>::value)
            {
                int32_t prefix_size = 0;
                if (!buffer.Read(prefix_size))
                {
                    return false;
                }

                already_read_size += sizeof(prefix_size);

                if (prefix_size != value.size())
                {
                    return false;
                }

                already_read_size += prefix_size;

                buffer.Consume(prefix_size);

                return true;

            }*/
            else if constexpr (std::is_array<RealType>::value)
            {
                constexpr size_t n = std::extent<std::remove_cvref_t<RealType>>::value;
                if (buffer.Size() >= n)
                {
                    buffer.Consume(n);
                    already_read_size += n;
                    return true;
                }

                return false;
            }
            else if constexpr (tmp::is_stdarray<RealType>::value)
            {
                constexpr size_t n = std::tuple_size<RealType>{};

                if (buffer.Size() >= n)
                {
                    buffer.Consume(n);
                    already_read_size += n;
                    return true;
                }

                return false;
            }
            else
            {
                static_assert(0, "Unsupported type");
            }
        }
    };

    template<DataBuffer::ByteOrderEndian endian, class Head, class... Tail>
    struct BatchReadDataSize<endian, Head, Tail...>
    {
        bool operator()(DataBuffer& buffer, size_t& already_read_size) const
        {
            return BatchReadDataSize<endian, Head>{}(buffer, already_read_size) && BatchReadDataSize<endian, Tail...>{}(buffer, already_read_size);
        }
    };

    template<DataBuffer::ByteOrderEndian endian = DataBuffer::ByteOrderEndian::kNative, class... Types>
    size_t GetBatchReadDataSize(DataBuffer& buffer)  noexcept
    {
        auto ptr = buffer.Data();
        size_t size = 0;
        if (!BatchReadDataSize<endian, Types...>{}(buffer, size))
        {
            assert(buffer.Data() - ptr == size);
            buffer.UnConsume(size);
            return 0;
        }

        assert(buffer.Data() - ptr == size);
        buffer.UnConsume(size);

        return size;
    }

}

inline void DataBufferTest()
{
    mp::DataBuffer buf(10, 128);
    int32_t i32 = 32;
    int16_t i16 = 16;
    int8_t i8 = 8;
    std::array<char, 10> array1;
    array1.fill('x');
    char array2[2] = { 'A','B' };
    std::string str = "hello";

    auto size = mp::GetBatchWriteDataSize(i32, i16, i8, array1, array2, str);
    auto b = mp::BatchWriteFront(buf, i32, i16, i8, array1, array2, str);
    assert(b);
    int32_t ii32 = 0;
    int16_t ii16 = 0;
    int8_t ii8 = 0;
    std::array<char, 10> array11;
    char array22[2] = {  };
    std::string str2;

    auto n = mp::GetBatchReadDataSize(buf, ii32, ii16, ii8, array11, array22, str2);
    assert(n == size);

    auto n2 = GetBatchReadDataSize<mp::DataBuffer::kNative, decltype(ii32), decltype(ii16), decltype(ii8), decltype(array11), decltype(array22), decltype(str2)>(buf);
    assert(n2 == n);

    auto ptr = buf.Data();
    mp::BatchRead(buf, ii32, ii16, ii8, array11, array22, str2);
    assert(buf.Data() - ptr == n);

    mp::tmp::TupleForeachReverse([](auto& v)
        {
            std::cout << v << "\n";
        }, std::make_tuple(ii32, ii16, ii8, str2));

    mp::tmp::TupleForeach([](auto& v)
        {
            std::cout << v << "\n";
        }, std::make_tuple(ii32, ii16, ii8, str2));

    int i = 0;
    mp::tmp::TupleForeachReverseWithReturnBool([&i](auto& v)
        {
            if (i++ > 0)
            {
                return false;
            }
            std::cout << v << "\n";
            return true;
        }, std::make_tuple(ii32, ii16, ii8, str2));
}