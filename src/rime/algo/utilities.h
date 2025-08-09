//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-01-30 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_UTILITIES_H_
#define RIME_UTILITIES_H_

#include <stdint.h>
// #include <boost/crc.hpp>
#include <rime/common.h>

namespace rime
{
    int CompareVersionString(const string &x, const string &y);

    class CRC32Calculator
    {
    public:
        CRC32Calculator()
        {
            // 初始化 CRC32 表
            for (uint32_t i = 0; i < 256; i++)
            {
                uint32_t crc = i;
                for (int j = 0; j < 8; j++)
                {
                    crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
                }
                table_[i] = crc;
            }
            Reset();
        }

        void Update(const void *data, size_t length)
        {
            const uint8_t *bytes = static_cast<const uint8_t *>(data);
            for (size_t i = 0; i < length; i++)
            {
                crc_ = table_[(crc_ ^ bytes[i]) & 0xFF] ^ (crc_ >> 8);
            }
        }

        uint32_t Finalize() const
        {
            return crc_ ^ 0xFFFFFFFF;
        }

        void Reset()
        {
            crc_ = 0xFFFFFFFF;
        }

    private:
        std::array<uint32_t, 256> table_;
        uint32_t crc_;
    };

    class ChecksumComputer
    {
    public:
        void ProcessFile(const string &file_name);
        uint32_t Checksum();

    private:
        // boost::crc_32_type crc_;
        CRC32Calculator crc_;
    };

    inline uint32_t Checksum(const string &file_name)
    {
        ChecksumComputer c;
        c.ProcessFile(file_name);
        return c.Checksum();
    }

} // namespace rime

#endif // RIME_UTILITIES_H_
