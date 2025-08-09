#ifndef RIME_PATH_H_
#define RIME_PATH_H_

#include <string>
#include <algorithm>
#include <cctype>
#include <vector>
#include <iterator>
#include <sys/stat.h>
#include <regex>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <utime.h>
#include <random>
#include <sstream>
#include <iomanip>
#include <functional>

namespace rime
{
    // 跨平台路径类实现
    class Path
    {
    public:
        // 目录迭代器类 - 现在只做前置声明
        class directory_iterator;

        Path() = default;
        Path(const char *path) : path_(normalize(path)) {}
        Path(const std::string &path) : path_(normalize(path)) {}

        // 检查路径是否为空
        bool empty() const
        {
            return path_.empty();
        }

        // 拼接路径
        Path operator/(const std::string &other) const
        {
            return Path(join(path_, normalize(other)));
        }

        // 就地拼接路径（右操作数为 std::string）
        Path &operator/=(const std::string &other)
        {
            path_ = join(path_, normalize(other));
            return *this;
        }

        // 就地拼接路径（右操作数为 Path 对象）
        Path &operator/=(const Path &other)
        {
            path_ = join(path_, other.path_);
            return *this;
        }

        // 添加针对 const char* 的重载
        Path &operator/=(const char *other)
        {
            *this /= std::string(other); // 委托给现有的 std::string 重载
            return *this;
        }
        // 相等运算符
        bool operator==(const Path &other) const
        {
            // 比较规范化后的路径字符串
            return path_ == other.path_;
        }

        // 不等运算符
        bool operator!=(const Path &other) const
        {
            return !(*this == other);
        }

        // 目录迭代器访问点
        inline directory_iterator directory_begin() const;
        inline static directory_iterator directory_end();

        // 转换为通用格式（统一斜杠）
        std::string generic_string() const
        {
            return path_;
        }

        // 检查是否有父路径
        bool has_parent_path() const
        {
            return path_.find('/') != std::string::npos;
        }

        // 转换为字符串
        std::string string() const
        {
            return path_;
        }

        // 转换为字符串（始终使用正斜杠）
        std::string generic_string_slash() const
        {
            return path_;
        }

        // 获取规范化的绝对路径（解析符号链接）
        Path canonical() const
        {
            if (path_.empty())
                return *this;

            // POSIX 实现
            char resolved[PATH_MAX];
            if (realpath(path_.c_str(), resolved))
            {
                return Path(resolved);
            }

            // 出错时返回原路径
            return *this;
        }

        // 检查路径是否存在
        bool exists() const
        {
            if (path_.empty())
            {
                return false;
            }

            // POSIX 实现
            struct stat stat_buf;
            return stat(path_.c_str(), &stat_buf) == 0;
        }

        // 检查是否是普通文件
        bool is_regular_file() const
        {
            if (path_.empty())
            {
                return false;
            }

            // 普通文件应满足：
            // 1. 不是目录
            // 2. 不是系统文件
            // 3. 不是设备文件

            // POSIX 实现
            struct stat stat_buf;
            if (stat(path_.c_str(), &stat_buf) != 0)
            {
                return false;
            }

            // 使用 S_ISREG 宏检查是否是普通文件
            return S_ISREG(stat_buf.st_mode);
        }

        // 检查路径是否是符号链接
        bool is_symlink() const
        {
            if (empty())
            {
                return false;
            }

            // POSIX 实现
            struct stat stat_buf;
            if (lstat(string().c_str(), &stat_buf) != 0)
            {
                return false;
            }

            return S_ISLNK(stat_buf.st_mode);
        }

        // 获取父路径
        Path parent_path() const
        {
            if (path_.empty())
            {
                return Path();
            }

            // 处理根目录情况
            if (path_ == "/")
            {
                return Path(); // 根目录没有父目录
            }

            // 找到最后一个路径分隔符
            size_t pos = path_.find_last_of('/');

            // 没有分隔符的情况
            if (pos == std::string::npos)
            {
                return Path(); // 没有父目录
            }

            // 处理以斜杠结尾的路径（如 "/home/user/")
            if (pos == path_.size() - 1)
            {
                // 移除结尾斜杠后重新查找
                std::string trimmed = path_.substr(0, path_.size() - 1);
                pos = trimmed.find_last_of('/');
                if (pos == std::string::npos)
                {
                    return Path();
                }
            }

            // 处理根目录的特殊情况（如 "/home" -> "/"）
            if (pos == 0)
            {
                return Path("/");
            }

            // 返回从开始到最后一个分隔符的子串
            return Path(path_.substr(0, pos));
        }

        // 获取文件名部分
        std::string filename() const
        {
            size_t pos = path_.find_last_of('/');
            if (pos == std::string::npos)
                return path_;
            return path_.substr(pos + 1);
        }

        // 转换为绝对路径（相对于给定根路径）
        Path absolute(const Path &base) const
        {
            if (is_absolute())
                return *this;
            return base / path_;
        }

        Path &replace_extension(const std::string &new_extension)
        {
            if (path_.empty())
                return *this;

            // 找到文件名部分
            size_t last_slash = path_.find_last_of('/');
            std::string filename = (last_slash == std::string::npos) ? path_ : path_.substr(last_slash + 1);

            // 找到扩展名位置
            size_t last_dot = filename.find_last_of('.');

            // 如果点号不在开头位置，则认为是扩展名
            if (last_dot != std::string::npos && last_dot > 0)
            {
                // 移除现有扩展名
                path_.resize(path_.size() - (filename.size() - last_dot));
            }

            // 添加新扩展名
            if (!new_extension.empty())
            {
                // 确保扩展名以点开头
                if (new_extension[0] != '.')
                {
                    path_ += '.';
                }
                path_ += new_extension;
            }

            return *this;
        }

        // 移除扩展名（无参数版本）
        Path &replace_extension()
        {
            if (path_.empty())
                return *this;

            // 找到文件名部分
            size_t last_slash = path_.find_last_of('/');
            std::string filename = (last_slash == std::string::npos) ? path_ : path_.substr(last_slash + 1);

            // 找到扩展名位置
            size_t last_dot = filename.find_last_of('.');

            // 处理隐藏文件（以点开头的文件）
            bool is_hidden = !filename.empty() && filename[0] == '.';
            size_t start = is_hidden ? 1 : 0;

            // 检查是否应移除扩展名：
            // 1. 有点号存在
            // 2. 点号不在起始位置（对于隐藏文件）
            // 3. 点号不是字符串的第一个字符（对于非隐藏文件）
            if (last_dot != std::string::npos && last_dot > start)
            {
                // 计算要移除的字符数（文件名中从点号开始到最后的部分）
                size_t chars_to_remove = filename.size() - last_dot;

                // 从完整路径中移除扩展名
                path_.resize(path_.size() - chars_to_remove);
            }

            return *this;
        }

        // 获取文件扩展名（包含点号）
        std::string extension() const
        {
            // 获取文件名部分
            std::string name = filename();

            // 处理隐藏文件（以点开头的文件）
            bool is_hidden = !name.empty() && name[0] == '.';
            size_t start = is_hidden ? 1 : 0;

            // 找到最后一个点号
            size_t last_dot = name.find_last_of('.');

            // 如果点号不在开头位置，则认为是扩展名
            if (last_dot != std::string::npos && last_dot > start)
            {
                return name.substr(last_dot);
            }

            return "";
        }

        // 检查是否是目录
        bool is_directory() const
        {
            if (path_.empty())
                return false;

            struct stat stat_buf;
            return stat(path_.c_str(), &stat_buf) == 0 &&
                   S_ISDIR(stat_buf.st_mode);
        }

        // 检查是否是相对路径
        bool is_relative() const
        {
            return !is_absolute();
        }

        // 将内部路径字符串转换为平台原生路径
        std::string native() const
        {
            return path_;
        }

        // 路径规范化
        static std::string normalize(const std::string &path)
        {
            if (path.empty())
                return "";

            std::string normalized = path;

            // 处理多个连续斜杠
            auto new_end = std::unique(normalized.begin(), normalized.end(),
                                       [](char l, char r)
                                       { return l == '/' && r == '/'; });
            normalized.erase(new_end, normalized.end());

            return normalized;
        }

        // 清空路径
        void clear()
        {
            path_.clear();
        }

    private:
        // 路径拼接实现
        static std::string join(const std::string &base, const std::string &path)
        {
            if (base.empty())
                return path;
            if (path.empty())
                return base;

            bool base_ends = (base.back() == '/');
            bool path_starts = (path.front() == '/');

            if (base_ends && path_starts)
            {
                return base + path.substr(1);
            }
            else if (!base_ends && !path_starts)
            {
                return base + '/' + path;
            }
            else
            {
                return base + path;
            }
        }

        // 检查是否是绝对路径
        bool is_absolute() const
        {
            return !path_.empty() && path_[0] == '/';
        }

        std::string path_;
    };

    // 目录迭代器类
    class Path::directory_iterator
    {
    public:
        struct direntry
        {
            rime::Path path;   // 文件路径
            std::string name;  // 文件名
            bool is_directory; // 是否是目录
        };

        directory_iterator() : state_(ENDED) {} // 默认构造为结束迭代器

        explicit directory_iterator(const Path &path) : base_path_(path), current_entry_()
        {
            open_directory(path);
            operator++(); // 读取第一个条目
        }

        // 添加 path() 方法返回当前目录项的完整路径
        const Path &path() const
        {
            return current_entry_.path;
        }

        // 前置递增运算符
        directory_iterator &operator++()
        {
            if (state_ == ENDED || state_ == ERROR)
                return *this;

            struct dirent *entry;
            while ((entry = readdir(dir_)) != nullptr)
            {
                // 跳过 "." 和 ".." 目录
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                    continue;

                current_entry_.name = entry->d_name;
                // 使用基础路径构建完整路径
                current_entry_.path = base_path_ / entry->d_name;

#ifdef _DIRENT_HAVE_D_TYPE
                if (entry->d_type != DT_UNKNOWN && entry->d_type != DT_LNK)
                {
                    current_entry_.is_directory = (entry->d_type == DT_DIR);
                }
                else
#endif
                {
                    // 文件类型未知，需要stat
                    struct stat stat_buf;
                    // 使用完整路径进行 stat
                    if (stat(current_entry_.path.string().c_str(), &stat_buf) == 0)
                    {
                        current_entry_.is_directory = S_ISDIR(stat_buf.st_mode);
                    }
                }
                break;
            }

            if (!entry)
            {
                close_directory();
                state_ = ENDED;
            }

            return *this;
        }

        const direntry &operator*() const { return current_entry_; }
        const direntry *operator->() const { return &current_entry_; }

        bool operator==(const directory_iterator &other) const
        {
            // 两个迭代器都结束或都不结束且路径相同
            return (state_ == ENDED && other.state_ == ENDED) ||
                   (state_ != ENDED && other.state_ != ENDED &&
                    current_entry_.path == other.current_entry_.path);
        }

        bool operator!=(const directory_iterator &other) const
        {
            return !operator==(other);
        }

        // 结束标记
        static directory_iterator end() { return directory_iterator(); }

        ~directory_iterator() { close_directory(); }

    private:
        enum
        {
            READY,
            ENDED,
            ERROR
        } state_;
        direntry current_entry_;
        Path base_path_; // 存储基础路径

        void open_directory(const Path &path)
        {
            if (path.empty() || !path.is_directory())
                throw std::runtime_error("Path is not a directory");

            dir_ = opendir(path.string().c_str());
            state_ = (dir_ == nullptr) ? ERROR : READY;
        }

        void close_directory()
        {
            if (dir_)
                closedir(dir_);
            dir_ = nullptr;
            state_ = ENDED;
        }

        DIR *dir_ = nullptr;
    };

    // 在 Path 类外部定义 directory_begin 和 directory_end
    Path::directory_iterator Path::directory_begin() const
    {
        return directory_iterator(*this);
    }

    Path::directory_iterator Path::directory_end()
    {
        return directory_iterator::end();
    }

    // 替代 boost::starts_with
    // 通用实现
    template <typename StringT>
    bool starts_with_impl(const StringT &str, const StringT &prefix)
    {
        return str.size() >= prefix.size() &&
               str.compare(0, prefix.size(), prefix) == 0;
    }

    // 窄字符串特化
    inline bool starts_with(const std::string &str, const std::string &prefix)
    {
        return starts_with_impl(str, prefix);
    }

    inline bool starts_with(const std::string &str, const char *prefix)
    {
        return starts_with(str, std::string(prefix));
    }

    // 宽字符串特化
    inline bool starts_with(const std::wstring &str, const std::wstring &prefix)
    {
        return starts_with_impl(str, prefix);
    }

    inline bool starts_with(const std::wstring &str, const wchar_t *prefix)
    {
        return starts_with(str, std::wstring(prefix));
    }

    // 混合类型支持
    inline bool starts_with(const std::wstring &str, const char *prefix)
    {
        std::wstring wprefix;
        while (*prefix)
        {
            wprefix += static_cast<wchar_t>(*prefix++);
        }
        return starts_with(str, wprefix);
    }

    inline bool starts_with(const std::string &str, const wchar_t *prefix)
    {
        std::string sprefix;
        while (*prefix)
        {
            sprefix += static_cast<char>(*prefix++);
        }
        return starts_with(str, sprefix);
    }

    // 替代 boost::ends_with
    inline bool ends_with(const std::string &str, const std::string &suffix)
    {
        return str.size() >= suffix.size() &&
               str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    // 检查文件或目录是否存在
    static bool Exists(const Path &path)
    {
        struct stat buffer;
        return (stat(path.generic_string_slash().c_str(), &buffer) == 0) &&
               S_ISREG(buffer.st_mode);
    }

    // 定义类似 boost::is_any_of 的谓词类
    class is_any_of
    {
    public:
        explicit is_any_of(const std::string &delimiters)
            : delimiters_(delimiters) {}

        bool operator()(char c) const
        {
            return delimiters_.find(c) != std::string::npos;
        }

    private:
        std::string delimiters_;
    };

    // 基础版本：去除字符串右侧空白字符
    inline void trim_right(std::string &str)
    {
        if (str.empty())
            return;

        size_t end = str.size();
        while (end > 0 && std::isspace(static_cast<unsigned char>(str[end - 1])))
        {
            --end;
        }

        str.resize(end);
    }

    // 模板化的右侧修剪函数（适用于谓词）
    template <typename Predicate>
    void trim_right_if(std::string &s, Predicate predicate)
    {
        // 从字符串末尾反向查找第一个不满足谓词的位置
        auto end = s.end();
        while (end != s.begin())
        {
            if (!predicate(*(end - 1)))
            {
                break;
            }
            --end;
        }
        s.erase(end, s.end());
    }

    // 优化的特化版本（针对常用分隔符）
    inline void trim_right_if(std::string &s, const rime::is_any_of &predicate)
    {
        auto end = s.end();
        while (end != s.begin())
        {
            if (!predicate(*(end - 1)))
            {
                break;
            }
            --end;
        }
        s.erase(end, s.end());
    }

    // 特化版本（针对标准库isspace）
    inline void trim_right_if(std::string &s, int (*predicate)(int))
    {
        auto end = s.end();
        while (end != s.begin())
        {
            if (!predicate(static_cast<unsigned char>(*(end - 1))))
            {
                break;
            }
            --end;
        }
        s.erase(end, s.end());
    }

    // 特定于字符集的修剪方法（不需要谓词）
    inline void trim_right_if(std::string &s, const char *delimiters)
    {
        rime::trim_right_if(s, rime::is_any_of(delimiters));
    }

    // 修剪字符串左侧空白字符
    inline void trim_left(std::string &str)
    {
        if (str.empty())
            return;

        size_t start = 0;
        while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start])))
        {
            ++start;
        }
        str.erase(0, start);
    }

    // 修剪字符串两侧空白字符
    inline void trim(std::string &str)
    {
        trim_left(str);
        trim_right(str);
    }

    // 实现与 boost::algorithm::split 参数顺序一致的函数
    inline void split(std::vector<std::string> &result_container,
                      const std::string &input_string,
                      const is_any_of &predicate,
                      bool compress = false)
    {
        result_container.clear();

        if (input_string.empty())
            return;

        auto start = input_string.begin();
        auto end = input_string.begin();

        while (start != input_string.end())
        {
            // 跳过分隔符
            while (start != input_string.end() && predicate(*start))
            {
                ++start;
            }

            if (start == input_string.end())
                break;

            // 找到 token 结束位置
            end = start;
            while (end != input_string.end() && !predicate(*end))
            {
                ++end;
            }

            // 添加 token
            result_container.push_back(std::string(start, end));

            // 如果压缩分隔符，跳过所有连续分隔符
            if (compress)
            {
                while (end != input_string.end() && predicate(*end))
                {
                    ++end;
                }
            }

            start = end;
        }
    }

    namespace adaptors
    {
        // 反向迭代器适配器
        template <typename Range>
        class reversed_range
        {
        public:
            // 使用条件类型处理常量性
            using iterator_type = typename std::conditional<
                std::is_const<Range>::value,
                typename Range::const_iterator,
                typename Range::iterator>::type;

            using reverse_iterator = std::reverse_iterator<iterator_type>;

            explicit reversed_range(Range &range) : range_(range) {}

            reverse_iterator begin()
            {
                return reverse_iterator(range_.end());
            }

            reverse_iterator end()
            {
                return reverse_iterator(range_.begin());
            }

            // 常量版本
            using const_iterator_type = typename Range::const_iterator;
            using const_reverse_iterator = std::reverse_iterator<const_iterator_type>;

            const_reverse_iterator begin() const
            {
                return const_reverse_iterator(range_.end());
            }

            const_reverse_iterator end() const
            {
                return const_reverse_iterator(range_.begin());
            }

        private:
            Range &range_;
        };

        // 适配器函数
        template <typename Range>
        reversed_range<Range> reverse(Range &range)
        {
            return reversed_range<Range>(range);
        }

        template <typename Range>
        reversed_range<const Range> reverse(const Range &range)
        {
            return reversed_range<const Range>(range);
        }

    }

    namespace algorithm
    {
        template <typename Container, typename Separator>
        std::string join(const Container &container, const Separator &separator)
        {
            // 处理空容器
            if (container.empty())
            {
                return "";
            }

            // 使用流连接元素
            std::ostringstream oss;
            auto it = std::begin(container);
            oss << *it;
            ++it;

            for (; it != std::end(container); ++it)
            {
                oss << separator << *it;
            }

            return oss.str();
        }
    }

    class StringFormatter
    {
    public:
        template <typename... Args>
        static std::string format(const std::string &fmt, Args... args)
        {
            std::ostringstream oss;
            format_impl(oss, fmt, args...);
            return oss.str();
        }

    private:
        template <typename T, typename... Args>
        static void format_impl(std::ostringstream &oss, const std::string &fmt,
                                T value, Args... args)
        {
            size_t pos = fmt.find('%');
            if (pos == std::string::npos)
            {
                oss << fmt;
                return;
            }

            // 输出 % 之前的内容
            oss << fmt.substr(0, pos);

            // 处理占位符
            if (pos + 1 < fmt.size())
            {
                char specifier = fmt[pos + 1];
                switch (specifier)
                {
                case 'd': // 整数
                    oss << value;
                    break;
                case 'f': // 浮点数
                    oss << std::fixed << static_cast<double>(value);
                    break;
                case 's': // 字符串
                    oss << value;
                    break;
                case '%': // 转义 %
                    oss << '%';
                    break;
                default:
                    oss << '%' << specifier;
                    break;
                }

                // 递归处理剩余部分
                format_impl(oss, fmt.substr(pos + 2), args...);
            }
            else
            {
                oss << '%';
            }
        }

        // 递归终止条件
        static void format_impl(std::ostringstream &oss, const std::string &fmt)
        {
            oss << fmt;
        }
    };

    class ScopeGuard
    {
    public:
        template <typename Func>
        ScopeGuard(Func &&func) : func_(std::forward<Func>(func)) {}

        ~ScopeGuard()
        {
            if (func_)
            {
                func_();
            }
        }

        // 禁止复制
        ScopeGuard(const ScopeGuard &) = delete;
        ScopeGuard &operator=(const ScopeGuard &) = delete;

        // 允许移动
        ScopeGuard(ScopeGuard &&other) noexcept
            : func_(std::move(other.func_))
        {
            other.func_ = nullptr;
        }

        ScopeGuard &operator=(ScopeGuard &&other) noexcept
        {
            if (this != &other)
            {
                func_ = std::move(other.func_);
                other.func_ = nullptr;
            }
            return *this;
        }

        // 取消守卫
        void dismiss()
        {
            func_ = nullptr;
        }

    private:
        std::function<void()> func_;
    };

    inline bool rename_file(const std::string &old_name, const std::string &new_name, std::error_code &ec)
    {
        ec.clear();

        if (std::rename(old_name.c_str(), new_name.c_str()) == 0)
        {
            return true;
        }

        ec = std::error_code(errno, std::system_category());
        return false;
    }

    inline void erase_last(std::string &str, const std::string &substr)
    {
        if (substr.empty())
            return;

        // 查找子字符串的最后一次出现
        size_t pos = str.rfind(substr);
        if (pos != std::string::npos)
        {
            // 删除找到的子字符串
            str.erase(pos, substr.length());
        }
    }

    inline bool is_regex_empty(const std::regex &re)
    {
        return re.mark_count() == 0; // 默认构造的 regex 返回 0
    }

    // 检查两个路径是否指向同一个文件/目录
    inline bool equivalent(const Path &p1, const Path &p2)
    {
        if (p1.empty() || p2.empty())
            return false;

        struct stat stat1, stat2;
        if (stat(p1.native().c_str(), &stat1) != 0)
            return false;
        if (stat(p2.native().c_str(), &stat2) != 0)
            return false;

        return (stat1.st_dev == stat2.st_dev) &&
               (stat1.st_ino == stat2.st_ino);
    }

    // 复制选项枚举（与 boost 保持一致）
    enum class copy_option
    {
        none,                // 默认行为（存在则失败）
        fail_if_exists,      // 如果目标存在则失败
        overwrite_if_exists, // 如果目标存在则覆盖
        update_existing      // 如果源文件较新则覆盖
    };
    // 实现 copy_file 方法
    inline bool copy_file(const Path &from, const Path &to, copy_option option = copy_option::none)
    {
        if (from.empty() || to.empty())
        {
            errno = EINVAL;
            return false;
        }

        // 检查源文件是否存在
        if (!from.exists())
        {
            errno = ENOENT;
            return false;
        }

        // 检查源文件是否是目录
        if (from.is_directory())
        {
            errno = EISDIR;
            return false;
        }

        // 处理复制选项
        if (to.exists())
        {
            switch (option)
            {
            case copy_option::none:
            case copy_option::fail_if_exists:
                errno = EEXIST;
                return false;

            case copy_option::overwrite_if_exists:
                // 删除现有文件
                if (std::remove(to.string().c_str()) != 0)
                {
                    return false;
                }
                break;

            case copy_option::update_existing:
                // 检查文件修改时间
                struct stat from_stat, to_stat;
                if (stat(from.string().c_str(), &from_stat) != 0)
                {
                    return false;
                }
                if (stat(to.string().c_str(), &to_stat) != 0)
                {
                    return false;
                }

                // 如果源文件不新于目标文件，则不复制
                if (from_stat.st_mtime <= to_stat.st_mtime)
                {
                    return true; // 不复制但返回成功
                }
                break;
            }
        }

        // 平台特定的复制实现
#ifdef __linux__
        // Linux 高效实现（使用 sendfile）
        int source = open(from.string().c_str(), O_RDONLY, 0);
        if (source == -1)
            return false;

        int dest = open(to.string().c_str(), O_WRONLY | O_CREAT, 0644);
        if (dest == -1)
        {
            close(source);
            return false;
        }

        struct stat stat_source;
        fstat(source, &stat_source);

        bool success = sendfile(dest, source, 0, stat_source.st_size) != -1;

        close(source);
        close(dest);
        return success;
#else
        // 通用实现（使用文件流）
        std::ifstream src(from.string(), std::ios::binary);
        if (!src.is_open())
            return false;

        std::ofstream dst(to.string(), std::ios::binary);
        if (!dst.is_open())
            return false;

        dst << src.rdbuf();

        src.close();
        dst.close();
        return true;
#endif
    }

    // 获取文件的最后修改时间
    inline std::time_t last_write_time(const Path &p)
    {
        if (p.empty())
        {
            throw std::system_error(std::make_error_code(std::errc::invalid_argument),
                                    "Cannot get last write time for empty path");
        }

        if (!p.exists())
        {
            throw std::system_error(std::make_error_code(std::errc::no_such_file_or_directory),
                                    "File does not exist: " + p.string());
        }

        // POSIX 实现
        struct stat stat_buf;
        if (stat(p.string().c_str(), &stat_buf) != 0)
        {
            throw std::system_error(errno, std::system_category(),
                                    "Failed to stat file: " + p.string());
        }

        // 返回最后修改时间
        return stat_buf.st_mtime;
    }

    // 设置文件的最后修改时间
    inline void last_write_time(const Path &p, std::time_t new_time)
    {
        if (p.empty())
        {
            throw std::system_error(std::make_error_code(std::errc::invalid_argument),
                                    "Cannot set last write time for empty path");
        }

        if (!p.exists())
        {
            throw std::system_error(std::make_error_code(std::errc::no_such_file_or_directory),
                                    "File does not exist: " + p.string());
        }

        // POSIX 实现
        struct utimbuf new_times;
        struct stat stat_buf;

        // 获取当前访问时间
        if (stat(p.string().c_str(), &stat_buf) != 0)
        {
            throw std::system_error(errno, std::system_category(),
                                    "Failed to stat file: " + p.string());
        }

        new_times.actime = stat_buf.st_atime; // 保持访问时间不变
        new_times.modtime = new_time;         // 设置新的修改时间

        if (utime(p.string().c_str(), &new_times) != 0)
        {
            throw std::system_error(errno, std::system_category(),
                                    "Failed to set file time: " + p.string());
        }
    }

    // 检查路径是否指向普通文件
    inline bool is_regular_file(const Path &p)
    {
        if (p.empty())
        {
            return false;
        }

        // 检查路径是否存在
        if (!p.exists())
        {
            return false;
        }

        // 检查是否是目录
        if (p.is_directory())
        {
            return false;
        }

        // POSIX 实现
        struct stat stat_buf;
        if (stat(p.string().c_str(), &stat_buf) != 0)
        {
            return false;
        }

        // 使用 S_ISREG 宏检查是否是普通文件
        return S_ISREG(stat_buf.st_mode);
    }

    // 递归创建目录
    inline bool create_directories(const Path &p)
    {
        if (p.empty())
        {
            return false;
        }

        // 如果目录已存在，直接返回成功
        if (p.exists() && p.is_directory())
        {
            return true;
        }

        // 分解路径为各个部分
        std::vector<std::string> parts;
        std::string current;
        for (char c : p.string())
        {
            if (c == '/')
            {
                if (!current.empty())
                {
                    parts.push_back(current);
                    current.clear();
                }
            }
            else
            {
                current += c;
            }
        }
        if (!current.empty())
        {
            parts.push_back(current);
        }

        // 逐级创建目录
        Path current_path;

        for (const auto &part : parts)
        {
            current_path /= part;

            // 如果当前路径已存在且是目录，继续下一级
            if (current_path.exists())
            {
                if (!current_path.is_directory())
                {
                    return false; // 存在但不是目录
                }
                continue;
            }

            // 创建当前目录
            if (mkdir(current_path.string().c_str(), 0755) != 0)
            {
                // 如果目录已存在（可能由其他进程创建），则继续
                if (errno != EEXIST)
                {
                    return false;
                }
            }
        }

        return true;
    }

    namespace uuids
    {

        // UUID 类
        class uuid
        {
        public:
            typedef unsigned char value_type;
            typedef value_type *iterator;
            typedef const value_type *const_iterator;

            // 默认构造函数（全零 UUID）
            uuid() : data_{0} {}

            // 从字节数组构造
            explicit uuid(const value_type bytes[16])
            {
                std::copy(bytes, bytes + 16, data_);
            }

            // 迭代器支持
            iterator begin() { return data_; }
            const_iterator begin() const { return data_; }
            iterator end() { return data_ + 16; }
            const_iterator end() const { return data_ + 16; }

            // 比较运算符
            bool operator==(const uuid &other) const
            {
                return std::equal(data_, data_ + 16, other.data_);
            }

            bool operator!=(const uuid &other) const
            {
                return !(*this == other);
            }

            bool operator<(const uuid &other) const
            {
                return std::lexicographical_compare(
                    data_, data_ + 16,
                    other.data_, other.data_ + 16);
            }

            // 转换为字符串
            std::string to_string() const
            {
                std::ostringstream oss;
                oss << std::hex << std::setfill('0');

                // 第一部分：8 字符
                for (int i = 0; i < 4; ++i)
                {
                    oss << std::setw(2) << static_cast<int>(data_[i]);
                }
                oss << '-';

                // 第二部分：4 字符
                for (int i = 4; i < 6; ++i)
                {
                    oss << std::setw(2) << static_cast<int>(data_[i]);
                }
                oss << '-';

                // 第三部分：4 字符
                for (int i = 6; i < 8; ++i)
                {
                    oss << std::setw(2) << static_cast<int>(data_[i]);
                }
                oss << '-';

                // 第四部分：4 字符
                for (int i = 8; i < 10; ++i)
                {
                    oss << std::setw(2) << static_cast<int>(data_[i]);
                }
                oss << '-';

                // 第五部分：12 字符
                for (int i = 10; i < 16; ++i)
                {
                    oss << std::setw(2) << static_cast<int>(data_[i]);
                }

                return oss.str();
            }

            // 获取原始字节数据
            const value_type *data() const { return data_; }
            size_t size() const { return 16; }

        private:
            value_type data_[16];
        };

        // UUID 随机生成器类
        class random_generator
        {
        public:
            // 构造函数
            random_generator()
            {
                // 使用硬件随机设备初始化随机数引擎
                std::random_device rd;
                engine_.seed(rd());
            }

            // 生成 UUID
            uuid operator()()
            {
                uuid::value_type bytes[16];

                // 生成随机字节
                for (int i = 0; i < 16; ++i)
                {
                    bytes[i] = static_cast<uuid::value_type>(distribution_(engine_));
                }

                // 设置 UUID 版本（版本4）
                bytes[6] = (bytes[6] & 0x0F) | 0x40; // 0100xxxx
                bytes[8] = (bytes[8] & 0x3F) | 0x80; // 10xxxxxx

                return uuid(bytes);
            }

        private:
            std::mt19937 engine_;                                     // Mersenne Twister 随机数引擎
            std::uniform_int_distribution<int> distribution_{0, 255}; // 字节分布
        };

    } // namespace uuids

    inline bool remove_file(const std::string &file_name)
    {
        if (file_name.empty())
        {
            return false;
        }

        // POSIX 实现
        if (unlink(file_name.c_str()) == 0)
        {
            return true;
        }
        int error = errno;
        // 如果文件不存在，视为成功
        if (error == ENOENT)
        {
            return true;
        }
        throw std::system_error(error, std::generic_category(),
                                "Cannot delete file: " + file_name);
    }

    // 删除文件（独立函数）
    inline bool remove_file(const Path &p)
    {
        if (p.empty())
        {
            return false;
        }

        if (!p.exists())
        {
            return true; // 不存在视为成功
        }

        if (p.is_directory())
        {
            return false; // 目录不能删除
        }

        if (unlink(p.string().c_str()) != 0)
        {
            return false;
        }

        return true;
    }

    // 带错误处理的版本
    inline bool remove_file(const Path &p, std::error_code &ec) noexcept
    {
        ec.clear();

        if (p.empty())
        {
            ec = std::make_error_code(std::errc::invalid_argument);
            return false;
        }

        if (!p.exists())
        {
            return true; // 不存在视为成功
        }

        if (p.is_directory())
        {
            ec = std::make_error_code(std::errc::is_a_directory);
            return false;
        }

        if (unlink(p.string().c_str()) != 0)
        {
            ec = std::error_code(errno, std::system_category());
            return false;
        }

        return true;
    }

    // 检查目录是否为空
    inline bool is_directory_empty(const Path &p, std::error_code &ec) noexcept
    {
        ec.clear();

        if (!p.exists() || !p.is_directory())
        {
            ec = std::make_error_code(std::errc::not_a_directory);
            return false;
        }

        DIR *dir = opendir(p.string().c_str());
        if (!dir)
        {
            ec = std::error_code(errno, std::system_category());
            return false;
        }

        int count = 0;
        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr)
        {
            // 跳过 "." 和 ".."
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }
            count++;
        }

        closedir(dir);
        return count == 0;
    }

    // 删除空目录
    inline bool remove_directory(const Path &p, std::error_code &ec) noexcept
    {
        ec.clear();

        if (p.empty())
        {
            ec = std::make_error_code(std::errc::invalid_argument);
            return false;
        }

        // 检查路径是否存在
        if (!p.exists())
        {
            ec = std::make_error_code(std::errc::no_such_file_or_directory);
            return false;
        }

        // 检查是否是目录
        if (!p.is_directory())
        {
            ec = std::make_error_code(std::errc::not_a_directory);
            return false;
        }

        // 检查目录是否为空
        if (!is_directory_empty(p, ec))
        {
            ec = std::make_error_code(std::errc::directory_not_empty);
            return false;
        }

        // 执行删除操作

        if (rmdir(p.string().c_str()) != 0)
        {
            ec = std::error_code(errno, std::system_category());
            return false;
        }

        return true;
    }

    // 递归删除目录及其内容（类似 boost::filesystem::remove_all）
    inline bool remove_all(const Path &p, std::error_code &ec) noexcept
    {
        ec.clear();

        if (p.empty())
        {
            ec = std::make_error_code(std::errc::invalid_argument);
            return false;
        }

        if (!p.exists())
        {
            return true; // 不存在视为成功
        }

        // 如果是文件，直接删除
        if (p.is_regular_file())
        {
            return remove_file(p);
        }

        // 如果是目录，递归删除内容
        if (p.is_directory())
        {
            // 遍历目录内容
            for (auto it = p.directory_begin(); it != Path::directory_end(); ++it)
            {
                if (!remove_all(it.path(), ec))
                {
                    return false;
                }
            }

            // 删除空目录
            return remove_directory(p, ec);
        }

        // 其他类型（符号链接等）
        if (unlink(p.string().c_str()) == 0)
        {
            return true;
        }
        ec = std::error_code(errno, std::system_category());

        return false;
    }

    // 跨设备重命名实现（复制+删除）
    inline bool rename_across_devices(const Path &from, const Path &to, std::error_code &ec) noexcept
    {
        ec.clear();

        try
        {
            // 如果是目录，递归复制
            if (from.is_directory())
            {
                // 创建目标目录
                if (!create_directories(to))
                {
                    return false;
                }

                // 复制目录内容
                for (auto it = from.directory_begin(); it != Path::directory_end(); ++it)
                {
                    Path source_item = it.path();
                    Path target_item = to / source_item.filename();

                    if (!rename_across_devices(source_item, target_item, ec))
                    {
                        return false;
                    }
                }

                // 删除源目录
                return remove_directory(from, ec);
            }
            else
            {
                // 复制文件
                if (!copy_file(from, to, rime::copy_option::overwrite_if_exists))
                {
                    return false;
                }

                // 删除源文件
                return remove_file(from);
            }
        }
        catch (const std::exception &e)
        {
            ec = std::make_error_code(std::errc::io_error);
            return false;
        }
    }

    // 重命名或移动文件/目录
    inline bool rename(const Path &from, const Path &to, std::error_code &ec) noexcept
    {
        ec.clear();

        if (from.empty() || to.empty())
        {
            ec = std::make_error_code(std::errc::invalid_argument);
            return false;
        }

        // 检查源路径是否存在
        if (!from.exists())
        {
            ec = std::make_error_code(std::errc::no_such_file_or_directory);
            return false;
        }

        // 尝试直接重命名
        // POSIX 实现
        if (std::rename(from.string().c_str(), to.string().c_str()) == 0)
        {
            return true;
        }

        ec = std::error_code(errno, std::system_category());

        // 如果直接重命名失败（可能是因为跨设备），尝试复制+删除
        if (ec.value() == EXDEV)
        { // 跨设备链接错误
            return rename_across_devices(from, to, ec);
        }

        return false;
    }

    // 检查路径是否是符号链接
    inline bool is_symlink(const Path &p)
    {
        if (p.empty())
        {
            return false;
        }

        // POSIX 实现
        struct stat stat_buf;
        if (lstat(p.string().c_str(), &stat_buf) != 0)
        {
            return false;
        }

        return S_ISLNK(stat_buf.st_mode);
    }

    // 带错误处理的版本
    inline bool is_symlink(const Path &p, std::error_code &ec) noexcept
    {
        ec.clear();

        if (p.empty())
        {
            ec = std::make_error_code(std::errc::invalid_argument);
            return false;
        }

        struct stat stat_buf;
        if (lstat(p.string().c_str(), &stat_buf) != 0)
        {
            ec = std::error_code(errno, std::system_category());
            return false;
        }

        return S_ISLNK(stat_buf.st_mode);
    }

    // 删除文件或空目录
    inline bool remove(const Path &p)
    {
        if (p.empty())
        {
            return false;
        }

        if (!p.exists())
        {
            return false; // 路径不存在
        }

        if (p.is_regular_file() || p.is_symlink())
        {
            return remove_file(p);
        }

        if (p.is_directory())
        {
            std::error_code ec;
            return remove_directory(p, ec);
        }

        return false; // 其他类型（如设备文件）
    }

    // 带错误处理的版本
    inline bool remove(const Path &p, std::error_code &ec) noexcept
    {
        ec.clear();

        if (p.empty())
        {
            ec = std::make_error_code(std::errc::invalid_argument);
            return false;
        }

        if (!p.exists())
        {
            ec = std::make_error_code(std::errc::no_such_file_or_directory);
            return false;
        }

        if (p.is_regular_file() || p.is_symlink())
        {
            return remove_file(p, ec);
        }

        if (p.is_directory())
        {
            return remove_directory(p, ec);
        }

        ec = std::make_error_code(std::errc::operation_not_supported);
        return false;
    }

    // 检查字符串是否包含子字符串（区分大小写）
    inline bool contains(const std::string &str, const std::string &sub)
    {
        if (sub.empty())
        {
            return true; // 空子字符串总是被包含
        }

        return str.find(sub) != std::string::npos;
    }
}

#endif
