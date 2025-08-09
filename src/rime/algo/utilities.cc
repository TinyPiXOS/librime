//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-01-30 GONG Chen <chen.sst@gmail.com>
//
#include <fstream>
// #include <boost/algorithm/string.hpp>
#include <rime/algo/utilities.h>

namespace rime
{
    // 通用字符串分割函数
    std::vector<std::string> split_string(const std::string &str, char delimiter)
    {
        std::vector<std::string> tokens;
        std::istringstream iss(str);
        std::string token;

        while (std::getline(iss, token, delimiter))
        {
            tokens.push_back(token);
        }

        return tokens;
    }

    // 支持多个分隔符的分割函数
    std::vector<std::string> split_string_any(const std::string &str, const std::string &delimiters)
    {
        std::vector<std::string> tokens;
        size_t start = 0;
        size_t end = 0;

        while (start < str.size())
        {
            // 找到下一个非分隔符位置
            start = str.find_first_not_of(delimiters, end);
            if (start == std::string::npos)
                break;

            // 找到下一个分隔符位置
            end = str.find_first_of(delimiters, start);
            if (end == std::string::npos)
                end = str.size();

            // 添加 token
            tokens.push_back(str.substr(start, end - start));
        }

        return tokens;
    }

    int CompareVersionString(const string &x, const string &y)
    {
        if (x.empty() && y.empty())
            return 0;
        if (x.empty())
            return -1;
        if (y.empty())
            return 1;

        // vector<string> xx, yy;
        // boost::split(xx, x, boost::is_any_of("."));
        // boost::split(yy, y, boost::is_any_of("."));

        std::vector<std::string> xx = split_string_any(x, ".");
        std::vector<std::string> yy = split_string_any(y, ".");

        size_t i = 0;
        for (; i < xx.size() && i < yy.size(); ++i)
        {
            int dx = atoi(xx[i].c_str());
            int dy = atoi(yy[i].c_str());
            if (dx != dy)
                return dx - dy;
            int c = xx[i].compare(yy[i]);
            if (c != 0)
                return c;
        }
        if (i < xx.size())
            return 1;
        if (i < yy.size())
            return -1;
        return 0;
    }

    void ChecksumComputer::ProcessFile(const string &file_name)
    {
        crc_.Reset();

        std::ifstream file(file_name, std::ios::binary);
        if (!file)
        {
            throw std::runtime_error("Cannot open file: " + file_name);
        }

        // 使用缓冲区读取文件，避免一次性加载大文件
        const size_t buffer_size = 65536; // 64KB
        std::vector<char> buffer(buffer_size);

        while (file)
        {
            file.read(buffer.data(), buffer_size);
            size_t bytes_read = file.gcount();
            crc_.Update(buffer.data(), bytes_read);
        }

        // std::ifstream fin(file_name.c_str());
        // string file_content((std::istreambuf_iterator<char>(fin)),
        //                     std::istreambuf_iterator<char>());
        // crc_.process_bytes(file_content.data(), file_content.length());
    }

    uint32_t ChecksumComputer::Checksum()
    {
        return crc_.Finalize();
        // return crc_.checksum();
    }

} // namespace rime
