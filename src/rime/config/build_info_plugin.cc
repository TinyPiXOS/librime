//
// Copyright RIME Developers
// Distributed under the BSD License
//
// #include <boost/filesystem.hpp>
#include <rime/service.h>
#include <rime/config/config_compiler.h>
#include <rime/config/config_types.h>
#include <rime/config/plugins.h>

#include <sys/stat.h>
#include <unistd.h>

namespace rime
{
    // 获取文件最后修改时间
    std::time_t get_last_write_time(const std::string &file_name)
    {
        // POSIX 实现 (Linux/macOS)
        struct stat file_stat;
        if (stat(file_name.c_str(), &file_stat) != 0)
        {
            return 0;
        }

        // 返回最后修改时间
        return file_stat.st_mtime;
    }

    bool
    BuildInfoPlugin::ReviewCompileOutput(
        ConfigCompiler *compiler, an<ConfigResource> resource)
    {
        return true;
    }

    bool BuildInfoPlugin::ReviewLinkOutput(
        ConfigCompiler *compiler, an<ConfigResource> resource)
    {
        auto build_info = (*resource)["__build_info"];
        build_info["rime_version"] = RIME_VERSION;
        auto timestamps = build_info["timestamps"];
        compiler->EnumerateResources([&](an<ConfigResource> resource)
                                     {
    if (!resource->loaded) {
      LOG(INFO) << "resource '" << resource->resource_id << "' not loaded.";
      timestamps[resource->resource_id] = 0;
      return;
    }
    auto file_name = resource->data->file_name();
    if (file_name.empty()) {
      LOG(WARNING) << "resource '" << resource->resource_id
                   << "' is not persisted.";
      timestamps[resource->resource_id] = 0;
      return;
    }
    // TODO: store as 64-bit number to avoid the year 2038 problem
    // timestamps[resource->resource_id] =
    //     (int) boost::filesystem::last_write_time(file_name); });
    //     return true;
    // }

    timestamps[resource->resource_id] =
        (int) get_last_write_time(file_name); });
        return true;
    }

} // namespace rime
