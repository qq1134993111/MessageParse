#pragma once
#include<cstdint>
#include<stdexcept>
#include<string.h>
#include<exception>

#include"EndianConversion.hpp"

//enum class ErrorCode : uint8_t
//{
//	kSuccess,
//	kBadMalloc,
//	kReadExceeds,
//};

namespace mp
{

    class DataBuffer
    {
    public:
        DataBuffer(uint32_t capacity = 1024) :capacity_(capacity), bufeer_(nullptr)
        {
            Clear();
            ExtendTo(capacity_);
        }

        DataBuffer(uint8_t* data, uint32_t size) :capacity_(size), bufeer_(nullptr)
        {
            Clear();
            ExtendTo(capacity_);
            Write(data, size);
        }

        DataBuffer(const DataBuffer& rhs) = delete;
        DataBuffer(const DataBuffer&& rhs) = delete;
        DataBuffer& operator=(const DataBuffer& rhs) = delete;
        DataBuffer&& operator=(const DataBuffer&& rhs) = delete;

        void Clear()
        {
            r_pos_ = 0;
            w_pos_ = 0;
        }

        void Write(const void* buf, uint32_t len)
        {
            if (w_pos_ + len > capacity_)
            {
                Extend(len);
            }

            if (buf != nullptr)
            {
                memcpy(bufeer_ + w_pos_, buf, len);
            }

            w_pos_ += len;

            //return ErrorCode::kSuccess;
        }

        void Read(void* buf, uint32_t len)
        {
            if (r_pos_ + len > w_pos_)
            {
                char sz_info[127] = { 0 };
                snprintf(sz_info, sizeof(sz_info) - 1, "DataBuffer Read exception:r_pos+len>w_pos,%d+%d>%d", r_pos_, len, w_pos_);
                throw std::runtime_error(sz_info);
            }

            if (buf != nullptr)
                memcpy(buf, bufeer_ + r_pos_, len);

            r_pos_ += len;

            //return ErrorCode::kSuccess;
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

        void Adjustment()
        {
            if (r_pos_ != 0)
            {
                memmove(bufeer_, bufeer_ + r_pos_, w_pos_ - r_pos_);
                w_pos_ -= r_pos_;
                r_pos_ = 0;
            }
        }

        uint8_t* GetReadPtr()
        {
            return bufeer_ + r_pos_;
        }

        void SetReadPtr(uint8_t* ptr)
        {
            if (ptr >= bufeer_ && ptr <= bufeer_ + capacity_)
            {
                r_pos_ = uint32_t(ptr - bufeer_);
            }
            else
            {
                char sz_info[127] = { 0 };
                snprintf(sz_info, sizeof(sz_info) - 1, "DataBuffer SetReadPtr exception:set ptr address %p not in [%p,%p]", ptr, bufeer_, bufeer_ + capacity_);
                throw std::runtime_error(sz_info);
            }
        };

        uint32_t GetReadPos()
        {
            return r_pos_;
        }

        void SetReadPos(uint32_t pos)
        {
            if (pos <= capacity_)
            {
                r_pos_ = pos;
            }
            else
            {
                char sz_info[127] = { 0 };
                snprintf(sz_info, sizeof(sz_info) - 1, "DataBuffer SetReadPos exception:set pos %d not in [%d,%d]", pos, 0, capacity_);
                throw std::runtime_error(sz_info);
            }
        }

        uint8_t* GetWritePtr()
        {
            return bufeer_ + w_pos_;
        }

        void SetWritePtr(uint8_t* ptr)
        {
            if (ptr >= bufeer_ && ptr <= bufeer_ + capacity_)
            {
                w_pos_ = (uint32_t)(ptr - bufeer_);
            }
            else
            {
                char sz_info[127] = { 0 };
                snprintf(sz_info, sizeof(sz_info) - 1, "DataBuffer SetWritePtr exception:set ptr address %p not in [%p,%p]", ptr, bufeer_, bufeer_ + capacity_);
                throw std::runtime_error(sz_info);
            }
        }

        uint32_t GetWritePos()
        {
            return w_pos_;
        }

        void SetWritePos(uint32_t pos)
        {
            if (pos <= capacity_)
            {
                w_pos_ = pos;
            }
            else
            {
                char sz_info[127] = { 0 };
                snprintf(sz_info, sizeof(sz_info) - 1, "DataBuffer SetReadPos exception:set pos %d not in [%d,%d]", pos, 0, capacity_);
                throw std::runtime_error(sz_info);
            }
        }

        uint32_t GetDataSize()
        {
            return  w_pos_ - r_pos_;
        }
        uint32_t GetCapacitySize()
        {
            return capacity_;
        }
        uint32_t GetAvailableSize()
        {
            return capacity_ - w_pos_;
        }

        void SetCapacitySize(uint32_t size)
        {
            ExtendTo(size);
            capacity_ = size;

            if (w_pos_ > size)
                w_pos_ = size;
            if (r_pos_ > size)
                r_pos_ = size;
        }

    protected:
        void Extend(uint32_t len)
        {
            auto capacity_tmp = w_pos_ + len;
            capacity_tmp += capacity_tmp >> 2;
            ExtendTo(capacity_tmp);
            capacity_ = capacity_tmp;
        }

        void ExtendTo(uint32_t len)
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
    private:
        uint32_t capacity_;//申请的空间大小
        uint8_t* bufeer_;//数据指针
        uint32_t r_pos_;//读位置
        uint32_t w_pos_;//写位置
    };

}