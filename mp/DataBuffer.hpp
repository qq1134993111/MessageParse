#pragma once
#include<cstdint>
#include<stdexcept>
#include<string.h>
#include<exception>
#include<assert.h>
#include <algorithm>
#include <string>
#include <string_view>
#include"EndianConversion.hpp"

namespace mp
{
    class  DataBuffer
    {
    public:
        enum  ByteOrderEndian
        {
            kNative = 0,
            kLittle = 1,
            kBig = 2
        };

        static const size_t kCheapPrependSize = 8;
        static const size_t kInitialSize = 256;

        explicit DataBuffer(size_t initial_size = kInitialSize, size_t reserved_prepend_size = kCheapPrependSize)
            : capacity_(reserved_prepend_size + initial_size)
            , read_index_(reserved_prepend_size)
            , write_index_(reserved_prepend_size)
            , reserved_prepend_size_(reserved_prepend_size)
        {
            buffer_ = new char[capacity_];
            assert(Size() == 0);
            assert(WritableBytes() == initial_size);
            assert(FrontWriteableBytes() == reserved_prepend_size);
        }

        explicit DataBuffer(const void* data, size_t size, size_t reserved_prepend_size = 0)
            : capacity_(reserved_prepend_size + size)
            , read_index_(reserved_prepend_size)
            , write_index_(reserved_prepend_size + size)
            , reserved_prepend_size_(reserved_prepend_size)
        {
            buffer_ = new char[capacity_];
            assert(Size() == size);
            assert(WritableBytes() == 0);
            assert(FrontWriteableBytes() == reserved_prepend_size);
        }

        ~DataBuffer()
        {
            delete[] buffer_;
            buffer_ = nullptr;
            capacity_ = 0;
        }

        void Swap(DataBuffer& rhs)
        {
            std::swap(buffer_, rhs.buffer_);
            std::swap(capacity_, rhs.capacity_);
            std::swap(read_index_, rhs.read_index_);
            std::swap(write_index_, rhs.write_index_);
            std::swap(reserved_prepend_size_, rhs.reserved_prepend_size_);
        }

        //read ptr
        char* Data()  noexcept
        {
            return buffer_ + read_index_;
        }

        const char* Data() const noexcept
        {
            return buffer_ + read_index_;
        }

        //readabe size
        size_t Size() const noexcept
        {
            assert(write_index_ >= read_index_);
            return write_index_ - read_index_;
        }

        //write ptr
        char* WritePtr() noexcept
        {
            return Begin() + write_index_;
        }

        const char* WritePtr() const noexcept
        {
            return Begin() + write_index_;
        }

        size_t WritableBytes() const noexcept
        {
            assert(capacity_ >= write_index_);
            return capacity_ - write_index_;
        }

        size_t FrontWriteableBytes() const noexcept
        {
            return read_index_;
        }

        size_t Capacity() const noexcept
        {
            return capacity_;
        }

        // Reset resets the buffer to be empty,
        // but it retains the underlying storage for use by future writes.
        // Reset is the same as Truncate(0).
        void Reset() noexcept
        {
            Truncate(0);
        }

        // Truncate discards all but the first n unread bytes from the buffer
        // but continues to use the same allocated storage.
        // It does nothing if n is greater than the length of the buffer.
        void Truncate(size_t n) noexcept
        {
            if (n == 0)
            {
                read_index_ = reserved_prepend_size_;
                write_index_ = reserved_prepend_size_;
            }
            else if (write_index_ > read_index_ + n)
            {
                write_index_ = read_index_ + n;
            }
        }

        void Reserve(size_t len)
        {
            if (capacity_ >= len + reserved_prepend_size_)
            {
                return;
            }

            Grow(len + reserved_prepend_size_);
        }

        void Shrink(size_t reserve = 0)
        {
            DataBuffer other(Size() + reserve);
            other.Write(Data(), Size());
            Swap(other);
        }

        void Commit(size_t n) noexcept
        {
            assert(n <= WritableBytes());
            if (n <= WritableBytes())
            {
                write_index_ += n;
            }
            else
            {
                write_index_ = capacity_;
            }
        }

        void Rever(size_t n)  noexcept
        {
            assert(n <= Size());
            if (write_index_ >= n)
            {
                write_index_ -= n;
            }
        }

        void Consume(std::size_t n) noexcept
        {
            if (n <= Size())
            {
                read_index_ += n;
            }
            else
            {
                Reset();
            }
        }

        std::string_view Prepare(std::size_t n)
        {
            EnsureWritableBytes(n);
            return std::string_view(WritePtr(), n);
        }

        void Adjustment()
        {
            if (read_index_ > reserved_prepend_size_)
            {
                auto data_size = Size();
                memmove(Begin() + reserved_prepend_size_, Data(), data_size);
                read_index_ = reserved_prepend_size_;
                write_index_ = read_index_ + data_size;
            }
        }
        // Write
    public:

        void Write(const void* buf, size_t len)
        {
            EnsureWritableBytes(len);
            memcpy(WritePtr(), buf, len);
            assert(write_index_ + len <= capacity_);
            write_index_ += len;
        }

        void Write(std::string_view sv)
        {
            EnsureWritableBytes(sv.size());
            memcpy(WritePtr(), sv.data(), sv.size());
            assert(write_index_ + sv.size() <= capacity_);
            write_index_ += sv.size();
        }

        template<size_t N>
        void Write(const std::array<char, N>& array)
        {
            Write(array.data(), N);
        }

        template<size_t N>
        void Write(const char(&array)[N])
        {
            Write(array, N);
        }

        template<ByteOrderEndian endian = ByteOrderEndian::kNative, typename T, typename std::enable_if <std::is_integral<T>::value, int >::type = 0>
        void Write(T value)
        {
            if constexpr (endian == ByteOrderEndian::kNative)
            {
                Write(&value, sizeof(value));
            }
            else if constexpr (endian == ByteOrderEndian::kLittle)
            {
                WriteIntegerLE(value);
            }
            else if constexpr (endian == ByteOrderEndian::kBig)
            {
                WriteIntegerBE(value);
            }
            else
            {
                static_assert(0, "endian error");
            }
        }


        template<typename T, typename std::enable_if <std::is_integral<T>::value, int >::type = 0>
        void WriteIntegerLE(T value)
        {
            static_assert(std::is_integral<T>::value, "must be Integer .");
            value = endian::htole(value);
            Write(&value, sizeof(value));
        }

        template<typename T, typename std::enable_if <std::is_integral<T>::value, int >::type = 0>
        void WriteIntegerBE(T value)
        {
            static_assert(std::is_integral<T>::value, "must be Integer .");
            value = endian::htobe(value);
            Write(&value, sizeof(value));
        }

        // Insert content, specified by the parameter, into the front of reading index
        bool WriteFront(const void* buf, size_t len)  noexcept
        {
            if (FrontWriteableBytes() < len)
            {
                return false;
            }

            read_index_ -= len;
            const char* p = static_cast<const char*>(buf);
            memcpy(Begin() + read_index_, p, len);
        }
        
        // Insert content, specified by the parameter, into the front of reading index
        bool WriteFront(std::string_view sv)  noexcept
        {
            if (FrontWriteableBytes() < sv.size())
            {
                return false;
            }

            read_index_ -= sv.size();
       
            memcpy(Begin() + read_index_, sv.data(),sv.size());
        }
        template<size_t N>
        void WriteFront(const std::array<char, N>& array)
        {
            WriteFront(array.data(), N);
        }

        template<size_t N>
        void WriteFront(const char(&array)[N])
        {
            WriteFront(array, N);
        }

        template<ByteOrderEndian endian = ByteOrderEndian::kNative, typename T, typename std::enable_if <std::is_integral<T>::value, int >::type = 0>
        void WriteFront(T value)
        {
            if constexpr (endian == ByteOrderEndian::kNative)
            {
                WriteFront(&value, sizeof(value));
            }
            else if constexpr (endian == ByteOrderEndian::kLittle)
            {
                value = endian::htole(value);
                WriteFront(&value, sizeof(value));
            }
            else if constexpr (endian == ByteOrderEndian::kBig)
            {
                value = endian::htobe(value);
                WriteFront(value);
            }
            else
            {
                static_assert(0, "endian error");
            }
        }

        //Read
    public:
        bool Peek(void* buf, size_t len)  noexcept
        {
            if (Size() < len)
            {
                return false;
            }

            memcpy(buf, Data(), len);
        }

        bool Read(void* buf, size_t len)  noexcept
        {
            if (Peek(buf, len))
            {
                Consume(len);
                return true;
            }

            return false;
        }

        template<size_t N>
        void Read(const std::array<char, N>& array)
        {
            Read(array.data(), N);
        }

        template<size_t N>
        void Read(const char(&array)[N])
        {
            Read(array, N);
        }

        template<ByteOrderEndian endian = ByteOrderEndian::kNative, typename T, typename std::enable_if <std::is_integral<T>::value, int >::type = 0 >
        void Read(T& value)
        {
            if constexpr (endian == ByteOrderEndian::kNative)
            {
                Read(&value, sizeof(value));
            }
            else if constexpr (endian == ByteOrderEndian::kLittle)
            {
                ReadIntegerLE(value);
            }
            else if constexpr (endian == ByteOrderEndian::kBig)
            {
                ReadIntegerBE(value);
            }
            else
            {
                static_assert(0, "endian error");
            }
        }

        template<typename T>
        void ReadIntegerLE(T& value)
        {
            static_assert(std::is_integral<T>::value, "must be Integer .");
            Read(&value, sizeof(value));
            value = endian::letoh(value);
        }

        template<typename T>
        void ReadIntegerBE(T& value)
        {
            static_assert(std::is_integral<T>::value, "must be Integer .");
            Read(&value, sizeof(value));
            value = endian::betoh(value);
        }

        // Helpers
    public:
        std::string ToString() const
        {
            return std::string(Data(), Size());
        }

        const char* FindCRLF() const  noexcept
        {
            const char* crlf = std::search(Data(), WritePtr(), kCRLF, kCRLF + 2);
            return crlf == WritePtr() ? nullptr : crlf;
        }

        const char* FindCRLF(const char* start) const  noexcept
        {
            assert(Data() <= start);
            assert(start <= WritePtr());
            const char* crlf = std::search(start, WritePtr(), kCRLF, kCRLF + 2);
            return crlf == WritePtr() ? nullptr : crlf;
        }

        const char* FindEOL() const  noexcept
        {
            const void* eol = memchr(Data(), '\n', Size());
            return static_cast<const char*>(eol);
        }

        const char* FindEOL(const char* start) const  noexcept
        {
            assert(Data() <= start);
            assert(start <= WritePtr());
            const void* eol = memchr(start, '\n', WritePtr() - start);
            return static_cast<const char*>(eol);
        }
    private:

        char* Begin() noexcept
        {
            return buffer_;
        }

        const char* Begin() const noexcept
        {
            return buffer_;
        }

        // Make sure there is enough memory space to append more data with length len
        void EnsureWritableBytes(size_t len)
        {
            if (WritableBytes() < len)
            {
                Grow(len);
            }

            assert(WritableBytes() >= len);
        }

        void Grow(size_t len)
        {
            if (WritableBytes() + FrontWriteableBytes() < len + reserved_prepend_size_)
            {
                //grow the capacity
                size_t n = (capacity_ << 1) + len;
                size_t data_size = Size();
                char* d = new char[n];
                memcpy(d + reserved_prepend_size_, Data(), data_size);
                write_index_ = data_size + reserved_prepend_size_;
                read_index_ = reserved_prepend_size_;
                capacity_ = n;
                delete[] buffer_;
                buffer_ = d;
            }
            else
            {
                // move readable data to the front, make space inside buffer
                assert(reserved_prepend_size_ < read_index_);
                size_t readable = Size();
                memmove(Begin() + reserved_prepend_size_, Data(), Size());
                read_index_ = reserved_prepend_size_;
                write_index_ = read_index_ + readable;
                assert(readable == Size());
                assert(WritableBytes() >= len);
            }
        }

    private:
        char* buffer_;
        size_t capacity_;
        size_t read_index_;
        size_t write_index_;
        size_t reserved_prepend_size_;
        static constexpr char kCRLF[] = "\r\n";
    };

}