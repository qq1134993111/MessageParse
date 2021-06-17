#pragma once
#include<cstdint>
#include<stdexcept>
#include<string.h>
#include<exception>
#include<assert.h>
#include"EndianConversion.hpp"

namespace mp
{

    class DataBuffer
    {
    public:
        DataBuffer(size_t capacity = 128) :capacity_(capacity), bufeer_(nullptr)
        {
            Clear();
            ExtendTo(capacity_);
        }

        DataBuffer(uint8_t* data, size_t size) :capacity_(size), bufeer_(nullptr)
        {
            Clear();
            ExtendTo(capacity_);
            Write(data, size);
        }

        DataBuffer(const DataBuffer& rhs) = delete;
        DataBuffer(const DataBuffer&& rhs) = delete;
        DataBuffer& operator=(const DataBuffer& rhs) = delete;
        DataBuffer&& operator=(const DataBuffer&& rhs) = delete;


        void Write(const void* buf, size_t len)
        {
            EnsureWritableBytes(len);

            if (buf != nullptr)
            {
                memcpy(bufeer_ + w_pos_, buf, len);
            }

            w_pos_ += len;
        }

        void Read(void* buf, size_t len)
        {
            if (ReadableBytes() < len)
            {
                char sz_info[127] = { 0 };
                snprintf(sz_info, sizeof(sz_info) - 1, "DataBuffer Read exception:r_pos+len>w_pos,%d+%d>%d", r_pos_, len, w_pos_);
                throw std::runtime_error(sz_info);
            }

            if (buf != nullptr)
                memcpy(buf, GetReadPtr(), len);

            r_pos_ += len;
        }

        template<typename T, typename std::enable_if <std::is_integral<T>::value, int >::type = 0 >
        void Write(T value)
        {
            Write(&value, sizeof(value));
        }

        template<typename T, typename std::enable_if <std::is_integral<T>::value, int >::type = 0 >
        void Read(T& value)
        {
            Read(&value, sizeof(value));
        }

        template<typename T>
        void WriteIntegerLE(T value)
        {
            static_assert(std::is_integral<T>::value, "must be Integer .");
            value = endian::htole(value);
            Write(&value, sizeof(value));
        }

        template<typename T>
        void ReadIntegerLE(T& value)
        {
            static_assert(std::is_integral<T>::value, "must be Integer .");
            Read(&value, sizeof(value));
            value = endian::letoh(value);
        }

        template<typename T>
        void WriteIntegerBE(T value)
        {
            static_assert(std::is_integral<T>::value, "must be Integer .");
            value = endian::htobe(value);
            Write(&value, sizeof(value));
        }

        template<typename T>
        void ReadIntegerBE(T& value)
        {
            static_assert(std::is_integral<T>::value, "must be Integer .");
            Read(&value, sizeof(value));
            value = endian::betoh(value);
        }

        template<size_t N>
        void WriteArray(const std::array<char, N>& array)
        {
            Write(array.data(), N);
        }

        template<size_t N>
        void ReadArray(std::array<char, N>& array)
        {
            Read(array.data(), N);
        }

        uint8_t* Data() const { return GetReadPtr(); }
        size_t GetDataSize() const
        {
            return ReadableBytes();
        }

        uint8_t* GetWritePtr()  const
        {
            return bufeer_ + w_pos_;
        }

        size_t WritableBytes() const
        {
            assert(capacity_ >= w_pos_);
            return capacity_ - w_pos_;
        }

        size_t GetCapacitySize() const
        {
            return capacity_;
        }

        void SetCapacitySize(size_t size)
        {
            Adjustment();
            ExtendTo(size);
            capacity_ = size;

            if (w_pos_ > size)
                w_pos_ = size;
            if (r_pos_ > size)
                r_pos_ = size;
        }

        void Shrink()
        {
            Adjustment();
            size_t data_size = GetDataSize();
            ExtendTo(data_size);
            capacity_ = data_size;
        }

        void Clear()
        {
            r_pos_ = 0;
            w_pos_ = 0;
        }

        void Adjustment()
        {
            if (r_pos_ != 0)
            {
                size_t data_len = ReadableBytes();
                memmove(bufeer_, GetReadPtr(), data_len);
                w_pos_ -= data_len;
                r_pos_ = 0;
            }
        }

    protected:
        uint8_t* GetReadPtr()  const
        {
            return bufeer_ + r_pos_;
        }

        uint8_t* ReadSkip(int64_t n)
        {
            if (n >= 0)
            {
                if (r_pos_ + n <= GetCapacitySize())
                {
                    r_pos_ += n;
                }
                else
                {
                    char sz_info[127] = { 0 };
                    snprintf(sz_info, sizeof(sz_info) - 1, "DataBuffer ReadSkip exception , r_pos_[%lld],n[%lld],capacity_[%lld]", r_pos_, n, capacity_);
                    throw std::runtime_error(sz_info);
                }
            }
            else
            {
                if (r_pos_ - n >= 0)
                {
                    r_pos_ -= n;
                }
                else
                {
                    char sz_info[127] = { 0 };
                    snprintf(sz_info, sizeof(sz_info) - 1, "DataBuffer ReadSkip exception , r_pos_[%lld],n[%lld],capacity_[%lld]", r_pos_, n, capacity_);
                    throw std::runtime_error(sz_info);
                }
            }

            return GetReadPtr();
        }

        uint8_t* WriteSkip(int64_t n)
        {
            if (n >= 0)
            {
                if (w_pos_ + n <= GetCapacitySize())
                {
                    w_pos_ += n;
                }
                else
                {
                    char sz_info[127] = { 0 };
                    snprintf(sz_info, sizeof(sz_info) - 1, "DataBuffer WriteSkip exception , r_pos_[%lld],n[%lld],capacity_[%lld]", w_pos_, n, capacity_);
                    throw std::runtime_error(sz_info);
                }
            }
            else
            {
                if (w_pos_ - n >= 0)
                {
                    w_pos_ -= n;
                }
                else
                {
                    char sz_info[127] = { 0 };
                    snprintf(sz_info, sizeof(sz_info) - 1, "DataBuffer WriteSkip exception , r_pos_[%lld],n[%lld],capacity_[%lld]", w_pos_, n, capacity_);
                    throw std::runtime_error(sz_info);
                }
            }

            return GetWritePtr();
        }

        size_t ReadableBytes() const
        {
            assert(w_pos_ >= r_pos_);
            return  w_pos_ - r_pos_;
        }

    protected:
        void ExtendTo(size_t len)
        {
            uint8_t* new_buf = static_cast<uint8_t*>(realloc(bufeer_, len));
            if (new_buf != nullptr)
            {
                bufeer_ = new_buf;
            }
            else
            {
                char sz_info[127] = { 0 };
                snprintf(sz_info, sizeof(sz_info) - 1, "DataBuffer malloc exception: Memory allocation %d bytes failure", len);
                throw std::runtime_error(sz_info);
            }
        }

        void Free()
        {
            if (bufeer_ != nullptr)
            {
                free(bufeer_);
                bufeer_ = nullptr;
                capacity_ = 0;
                w_pos_ = 0;
                r_pos_ = 0;
            }
        }

        void Grow(size_t len)
        {
            if (WritableBytes() + PrependableBytes() < len)
            {
                //grow the capacity
                size_t n = (capacity_ << 1) + len;
                ExtendTo(n);
                capacity_ = n;
            }
            else
            {
                Adjustment();
                assert(WritableBytes() >= len);
            }
        }

        void EnsureWritableBytes(size_t len)
        {
            if (WritableBytes() < len)
            {
                Grow(len);
            }

            assert(WritableBytes() >= len);
        }

        size_t PrependableBytes() const
        {
            return r_pos_;
        }
    private:
        size_t capacity_;//申请的空间大小
        uint8_t* bufeer_;//数据指针
        size_t r_pos_;//读位置
        size_t w_pos_;//写位置
    };

}