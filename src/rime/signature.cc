//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-02-21 GONG Chen <chen.sst@gmail.com>
//
#include <time.h>
// #include <boost/algorithm/string.hpp>
#include <rime/config.h>
#include <rime/deployer.h>
#include <rime/signature.h>

namespace rime
{

    bool Signature::Sign(Config *config, Deployer *deployer)
    {
        if (!config)
            return false;
        config->SetString(key_ + "/generator", generator_);
        time_t now = time(NULL);
        string time_str(ctime(&now));

        // 移除换行符和多余空白
        if (!time_str.empty() && time_str.back() == '\n')
        {
            time_str.pop_back();
        }
        rime::trim(time_str); // 使用自定义trim函数

        config->SetString(key_ + "/modified_time", time_str);
        config->SetString(key_ + "/distribution_code_name",
                          deployer->distribution_code_name);
        config->SetString(key_ + "/distribution_version",
                          deployer->distribution_version);
        config->SetString(key_ + "/rime_version", RIME_VERSION);
        return true;
    }

} // namespace rime
