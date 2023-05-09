#pragma once
#include <assert.h>
#include <cstdint>
#include <string.h>
// #include <stdexcept>
#include "EndianConversion.hpp"
#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

namespace mp
{
    class DataBuffer
    {
    public:
        enum class ByteOrder : uint8_t
        {
            kNative = 0,       // 本机的字节序
            kLittleEndian = 1, // 小端序
            kBigEndian = 2,    // 网络字节序
            kRuntime = 3       // 运行时设置的字节序
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
            assert(PrependableBytes() == reserved_prepend_size);
        }

        DataBuffer(const DataBuffer&) = delete;

        DataBuffer& operator=(const DataBuffer&) = delete;

        DataBuffer(DataBuffer&& other) noexcept
            : buffer_(other.buffer_)
            , capacity_(other.capacity_)
            , read_index_(other.read_index_)
            , write_index_(other.write_index_)
            , reserved_prepend_size_(other.reserved_prepend_size_)
            , endian_(other.endian_)
        {
            other.buffer_ = nullptr;
            other.capacity_ = 0;
            other.read_index_ = other.write_index_ = other.reserved_prepend_size_;
            other.endian_ = ByteOrder::kNative;
        }

        DataBuffer& operator=(DataBuffer&& other) noexcept
        {

            buffer_ = other.buffer_;
            capacity_ = other.capacity_;
            read_index_ = other.read_index_;
            write_index_ = other.write_index_;
            reserved_prepend_size_ = other.reserved_prepend_size_;
            endian_ = other.endian_;

            other.buffer_ = nullptr;
            other.capacity_ = 0;
            other.read_index_ = other.write_index_ = other.reserved_prepend_size_;
            other.endian_ = ByteOrder::kNative;

            return *this;
        }

        ~DataBuffer()
        {
            delete[] buffer_;
            buffer_ = nullptr;
            capacity_ = 0;
        }

        void Swap(DataBuffer& rhs) noexcept
        {
            std::swap(buffer_, rhs.buffer_);
            std::swap(capacity_, rhs.capacity_);
            std::swap(read_index_, rhs.read_index_);
            std::swap(write_index_, rhs.write_index_);
            std::swap(reserved_prepend_size_, rhs.reserved_prepend_size_);
            std::swap(endian_, rhs.endian_);
        }

        // read ptr
        char* Data() noexcept
        {
            return buffer_ + read_index_;
        }

        const char* Data() const noexcept
        {
            return buffer_ + read_index_;
        }

        // readabe size
        size_t Size() const noexcept
        {
            assert(write_index_ >= read_index_);
            return write_index_ - read_index_;
        }

        // write ptr
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

        size_t PrependableBytes() const noexcept
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
            other.SetEndian(endian_);
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

        void ReverCommit(size_t n) noexcept
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

        void ReverConsume(std::size_t n) noexcept
        {
            if (read_index_ >= n)
            {
                read_index_ -= n;
            }
            else
            {
                read_index_ = 0;
            }
        }

        std::string_view Prepare(std::size_t n)
        {
            EnsureWritableBytes(n);
            return std::string_view(WritePtr(), n);
        }

        void Adjustment() noexcept
        {
            if (read_index_ > reserved_prepend_size_)
            {
                auto data_size = Size();
                memmove(Begin() + reserved_prepend_size_, Data(), data_size);
                read_index_ = reserved_prepend_size_;
                write_index_ = read_index_ + data_size;
            }
        }

        void SetEndian(ByteOrder byte_order) noexcept
        {
            endian_ = byte_order;
        }

    public:
        // Write
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

        template <size_t N>
        void Write(const std::array<char, N>& array)
        {
            Write(array.data(), N);
        }

        template <size_t N>
        void Write(const char(&array)[N])
        {
            Write(array, N);
        }

        template <ByteOrder endian = ByteOrder::kRuntime, typename T,
            typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
        void Write(T value)
        {
            if constexpr (endian == ByteOrder::kNative)
            {
                WriteInteger(value);
            }
            else if constexpr (endian == ByteOrder::kLittleEndian)
            {
                WriteIntegerLE(value);
            }
            else if constexpr (endian == ByteOrder::kBigEndian)
            {
                WriteIntegerBE(value);
            }
            else if constexpr (endian == ByteOrder::kRuntime)
            {
                if (endian_ == ByteOrder::kBigEndian)
                {
                    WriteIntegerBE(value);
                }
                else if (endian_ == ByteOrder::kLittleEndian)
                {
                    WriteIntegerLE(value);
                }
                else
                {
                    WriteInteger(value);
                }
            }
            else
            {
                static_assert(0, "endian error");
            }
        }

        template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
        void WriteInteger(T value)
        {
            static_assert(std::is_integral<T>::value, "must be Integer .");
            Write(&value, sizeof(value));
        }

        template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
        void WriteIntegerLE(T value)
        {
            static_assert(std::is_integral<T>::value, "must be Integer .");
            value = endian::htole(value);
            Write(&value, sizeof(value));
        }

        template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
        void WriteIntegerBE(T value)
        {
            static_assert(std::is_integral<T>::value, "must be Integer .");
            value = endian::htobe(value);
            Write(&value, sizeof(value));
        }
        // Append
        void Append(const char* /*restrict*/ d, size_t len)
        {
            Write(d, len);
        }

        void Append(const void* /*restrict*/ d, size_t len)
        {
            Write(d, len);
        }

        void AppendInt64(int64_t x)
        {
            Write<ByteOrder::kRuntime>(x);
        }

        void AppendInt32(int32_t x)
        {
            Write<ByteOrder::kRuntime>(x);
        }

        void AppendInt16(int16_t x)
        {
            Write<ByteOrder::kRuntime>(x);
        }

        void AppendInt8(int8_t x)
        {
            Write(&x, sizeof x);
        }

        // WriteFront
        bool WriteFront(const void* buf, size_t len) noexcept
        {
            if (PrependableBytes() < len)
            {
                return false;
            }

            read_index_ -= len;
            const char* p = static_cast<const char*>(buf);
            memcpy(Begin() + read_index_, p, len);
            return true;
        }

        bool WriteFront(std::string_view sv) noexcept
        {
            if (PrependableBytes() < sv.size())
            {
                return false;
            }

            read_index_ -= sv.size();

            memcpy(Begin() + read_index_, sv.data(), sv.size());

            return true;
        }
        template <size_t N>
        bool WriteFront(const std::array<char, N>& array) noexcept
        {
            return WriteFront(array.data(), N);
        }

        template <size_t N>
        bool WriteFront(const char(&array)[N]) noexcept
        {
            return WriteFront(array, N);
        }

        template <ByteOrder endian = ByteOrder::kRuntime, typename T,
            typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
        bool WriteFront(T value) noexcept
        {
            if constexpr (endian == ByteOrder::kNative)
            {
                return WriteFront(&value, sizeof(value));
            }
            else if constexpr (endian == ByteOrder::kLittleEndian)
            {
                value = endian::htole(value);
                return WriteFront(&value, sizeof(value));
            }
            else if constexpr (endian == ByteOrder::kBigEndian)
            {
                value = endian::htobe(value);
                return WriteFront(value);
            }
            else if constexpr (endian == ByteOrder::kRuntime)
            {
                if (endian_ == ByteOrder::kBigEndian)
                {
                    value = endian::htobe(value);
                    return WriteFront(value);
                }
                else if (endian_ == ByteOrder::kLittleEndian)
                {
                    value = endian::htole(value);
                    return WriteFront(&value, sizeof(value));
                }
                else
                {
                    return WriteFront(&value, sizeof(value));
                }
            }
            else
            {
                static_assert(0, "endian error");
            }
            return false;
        }

        // PrependInt
        void PrependInt64(int64_t x)
        {
            WriteFront<ByteOrder::kRuntime>(x);
        }

        void PrependInt32(int32_t x)
        {
            WriteFront<ByteOrder::kRuntime>(x);
        }

        void PrependInt16(int16_t x)
        {
            WriteFront<ByteOrder::kRuntime>(x);
        }

        void PrependInt8(int8_t x)
        {
            WriteFront(&x, sizeof x);
        }

    public:
        // Peek
        bool Peek(void* buf, size_t len) noexcept
        {
            if (Size() < len)
            {
                return false;
            }

            memcpy(buf, Data(), len);

            return true;
        }

        template <size_t N>
        bool Peek(std::array<char, N>& array) noexcept
        {
            return Peek(array.data(), N);
        }

        template <size_t N>
        bool Peek(char(&array)[N]) noexcept
        {
            return Peek(array, N);
        }

        template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
        bool PeekIntegerLE(T& value) noexcept
        {
            static_assert(std::is_integral<T>::value, "must be Integer .");

            if (Peek(&value, sizeof(T)))
            {
                value = endian::letoh(value);
                return true;
            }

            return false;
        }

        template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
        std::optional<T> PeekIntegerLE() noexcept
        {
            static_assert(std::is_integral<T>::value, "must be Integer .");
            T value = 0;
            if (PeekIntegerLE(value))
                return value;
            return {};
        }

        template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
        bool PeekIntegerBE(T& value) noexcept
        {
            static_assert(std::is_integral<T>::value, "must be Integer .");

            if (Peek(&value, sizeof(T)))
            {
                value = endian::betoh(value);
                return true;
            }

            return false;
        }

        template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
        std::optional<T> PeekIntegerBE() noexcept
        {
            static_assert(std::is_integral<T>::value, "must be Integer .");
            T value = 0;
            if (PeekIntegerLE(value))
                return value;
            return {};
        }

        template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
        bool PeekInteger(T& value) noexcept
        {
            static_assert(std::is_integral<T>::value, "must be Integer .");

            return Peek(&value, sizeof(T));
        }

        template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
        std::optional<T> PeekInteger() noexcept
        {
            static_assert(std::is_integral<T>::value, "must be Integer .");
            T value = 0;
            if (Peek(&value, sizeof(T)))
            {
                return value;
            }

            return {};
        }

        template <typename T, ByteOrder endian = ByteOrder::kRuntime,
            typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
        std::optional<T> Peek() noexcept
        {
            if constexpr (endian == ByteOrder::kNative)
            {
                return PeekInteger<T>();
            }
            else if constexpr (endian == ByteOrder::kLittleEndian)
            {
                return PeekIntegerLE<T>();
            }
            else if constexpr (endian == ByteOrder::kBigEndian)
            {
                return PeekIntegerBE<T>();
            }
            else if constexpr (endian == ByteOrder::kRuntime)
            {
                if (endian_ == ByteOrder::kBigEndian)
                {
                    return PeekIntegerBE<T>();
                }
                else if (endian_ == ByteOrder::kLittleEndian)
                {
                    return PeekIntegerLE<T>();
                }
                else
                {
                    return PeekInteger<T>();
                }
            }
            else
            {
                static_assert(0, "endian error");
            }
        }

        template <ByteOrder endian = ByteOrder::kRuntime, typename T,
            typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
        bool Peek(T& value) noexcept
        {
            if constexpr (endian == ByteOrder::kNative)
            {
                return PeekInteger(value);
            }
            else if constexpr (endian == ByteOrder::kLittleEndian)
            {
                return ReadIntegerLE(value);
            }
            else if constexpr (endian == ByteOrder::kBigEndian)
            {
                return ReadIntegerBE(value);
            }
            else if constexpr (endian == ByteOrder::kRuntime)
            {
                if (endian_ == ByteOrder::kBigEndian)
                {
                    return PeekIntegerBE(value);
                }
                else if (endian_ == ByteOrder::kLittleEndian)
                {
                    return PeekIntegerLE(value);
                }
                else
                {
                    return PeekInteger(value);
                }
            }
            else
            {
                static_assert(0, "endian error");
            }
        }

        // PeekInt
        std::optional<int64_t> PeekInt64() noexcept
        {
            // assert(Size() >= sizeof(int64_t));
            return Peek<int64_t>();
        }

        std::optional<int32_t> PeekInt32() noexcept
        {
            // assert(Size() >= sizeof(int32_t));
            return Peek<int32_t, ByteOrder::kRuntime>();
        }

        std::optional<int16_t> PeekInt16() noexcept
        {
            // assert(Size() >= sizeof(int16_t));
            return Peek<int16_t, ByteOrder::kRuntime>();
        }

        std::optional<int8_t> PeekInt8() noexcept
        {
            assert(Size() >= sizeof(int8_t));
            return Peek<int8_t, ByteOrder::kRuntime>();
        }

    public:
        // Read
        bool Read(void* buf, size_t len) noexcept
        {
            if (Peek(buf, len))
            {
                Consume(len);
                return true;
            }

            return false;
        }

        template <size_t N>
        bool Read(std::array<char, N>& array) noexcept
        {
            return Read(array.data(), N);
        }

        template <size_t N>
        bool Read(char(&array)[N]) noexcept
        {
            return Read(array, N);
        }

        template <typename T>
        bool ReadInteger(T& value) noexcept
        {
            static_assert(std::is_integral<T>::value, "must be Integer .");
            return Read(&value, sizeof(T));
        }

        template <typename T>
        bool ReadIntegerLE(T& value) noexcept
        {
            static_assert(std::is_integral<T>::value, "must be Integer .");
            if (Read(&value, sizeof(value)))
            {
                value = endian::letoh(value);
                return true;
            }

            return false;
        }

        template <typename T>
        bool ReadIntegerBE(T& value) noexcept
        {
            static_assert(std::is_integral<T>::value, "must be Integer .");
            if (Read(&value, sizeof(value)))
            {
                value = endian::betoh(value);
                return true;
            }
            return false;
        }

        template <ByteOrder endian = ByteOrder::kRuntime, typename T,
            typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
        bool Read(T& value) noexcept
        {
            if constexpr (endian == ByteOrder::kNative)
            {
                return ReadInteger(value);
            }
            else if constexpr (endian == ByteOrder::kLittleEndian)
            {
                return ReadIntegerLE(value);
            }
            else if constexpr (endian == ByteOrder::kBigEndian)
            {
                return ReadIntegerBE(value);
            }
            else if constexpr (endian == ByteOrder::kRuntime)
            {
                if (endian_ == ByteOrder::kBigEndian)
                {
                    return ReadIntegerBE(value);
                }
                else if (endian_ == ByteOrder::kLittleEndian)
                {
                    return ReadIntegerLE(value);
                }
                else
                {
                    return ReadInteger(value);
                }
            }
            else
            {
                static_assert(0, "endian error");
            }
        }

        // Helpers
    public:
        // ToText appends char '\0' to buffer to convert the underlying data to a c-style string text.
        // It will not change the length of buffer.
        void ToText()
        {
            AppendInt8('\0');
            ReverCommit(1);
        }

        template <class T, class = typename std::enable_if<std::is_constructible<T, void*, std::size_t>::value>::type>
        operator T() const noexcept
        {
            return T{ Data(), Size() };
        }

        std::string ToString() const
        {
            return std::string(Data(), Size());
        }

        const char* FindCRLF() const noexcept
        {
            const char* crlf = std::search(Data(), WritePtr(), kCRLF, kCRLF + 2);
            return crlf == WritePtr() ? nullptr : crlf;
        }

        const char* FindCRLF(const char* start) const noexcept
        {
            assert(Data() <= start);
            assert(start <= WritePtr());
            const char* crlf = std::search(start, WritePtr(), kCRLF, kCRLF + 2);
            return crlf == WritePtr() ? nullptr : crlf;
        }

        const char* FindEOL() const noexcept
        {
            const void* eol = memchr(Data(), '\n', Size());
            return static_cast<const char*>(eol);
        }

        const char* FindEOL(const char* start) const noexcept
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
            if (WritableBytes() + PrependableBytes() < len + reserved_prepend_size_)
            {
                // grow the capacity
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
        ByteOrder endian_ = ByteOrder::kNative;
        static constexpr char kCRLF[] = "\r\n";
    };

} // namespace mp