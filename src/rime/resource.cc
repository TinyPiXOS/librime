//
// Copyright RIME Developers
// Distributed under the BSD License
//

// #include <boost/algorithm/string.hpp>
// #include <boost/filesystem.hpp>
#include <rime/resource.h>

namespace rime
{
    string ResourceResolver::ToResourceId(const string &file_path) const
    {
        // string path_string = boost::filesystem::path(file_path).generic_string();
        // bool has_prefix = boost::starts_with(path_string, type_.prefix);
        // bool has_suffix = boost::ends_with(path_string, type_.suffix);
        // size_t start = (has_prefix ? type_.prefix.length() : 0);
        // size_t end = path_string.length() - (has_suffix ? type_.suffix.length() : 0);
        // return path_string.substr(start, end);

        Path path(file_path);
        std::string path_string = path.generic_string();

        // 检查前缀
        bool has_prefix = starts_with(path_string, type_.prefix);
        // 检查后缀
        bool has_suffix = ends_with(path_string, type_.suffix);

        // 计算开始和结束位置
        size_t start = (has_prefix ? type_.prefix.length() : 0);
        size_t end = path_string.length() - (has_suffix ? type_.suffix.length() : 0);

        // 返回资源ID
        return path_string.substr(start, end - start);
    }

    string ResourceResolver::ToFilePath(const string &resource_id) const
    {
        // boost::filesystem::path file_path(resource_id);
        // bool missing_prefix = !file_path.has_parent_path() &&
        //                       !boost::starts_with(resource_id, type_.prefix);
        // bool missing_suffix = !boost::ends_with(resource_id, type_.suffix);
        // return (missing_prefix ? type_.prefix : "") + resource_id +
        //        (missing_suffix ? type_.suffix : "");

        Path file_path(resource_id);
        std::string path_str = file_path.generic_string();

        // 检查缺失的前缀后缀
        bool missing_prefix = !file_path.has_parent_path() &&
                              !starts_with(path_str, type_.prefix);
        bool missing_suffix = !ends_with(path_str, type_.suffix);

        // 构建完整路径
        return (missing_prefix ? type_.prefix : "") + path_str +
               (missing_suffix ? type_.suffix : "");
    }

    // boost::filesystem::path ResourceResolver::ResolvePath(const string &resource_id)
    rime::Path ResourceResolver::ResolvePath(const string &resource_id)
    {
        // 构建相对路径
        std::string relative_path = type_.prefix + resource_id + type_.suffix;

        // 返回相对于根目录的绝对路径
        return Path(relative_path).absolute(root_path_);

        // return boost::filesystem::absolute(
        //     boost::filesystem::path(type_.prefix + resource_id + type_.suffix),
        //     root_path_);
    }

    // boost::filesystem::path FallbackResourceResolver::ResolvePath(const string &resource_id)
    rime::Path FallbackResourceResolver::ResolvePath(const string &resource_id)
    {
        auto default_path = ResourceResolver::ResolvePath(resource_id);

        if (Exists(default_path))
        {
            return default_path;
        }

        // 尝试回退路径（如果设置了回退根路径）
        if (!fallback_root_path_.empty())
        {
            // 构建资源在回退目录中的路径
            Path fallback_path = fallback_root_path_ /
                                 (type_.prefix + resource_id + type_.suffix);

            // 检查回退路径是否存在
            if (Exists(fallback_path))
            {
                return fallback_path;
            }
        }

        // if (!boost::filesystem::exists(default_path))
        // {
        //     auto fallback_path = boost::filesystem::absolute(
        //         boost::filesystem::path(type_.prefix + resource_id + type_.suffix),
        //         fallback_root_path_);
        //     if (boost::filesystem::exists(fallback_path))
        //     {
        //         return fallback_path;
        //     }
        // }
        return default_path;
    }

} // namespace rime
