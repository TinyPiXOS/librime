//
// Copyright RIME Developers
// Distributed under the BSD License
//
#ifndef RIME_CUSTOMIZER_H_
#define RIME_CUSTOMIZER_H_

// #include <boost/filesystem.hpp>

#include <rime/rimePath.h>

namespace rime
{

    class Customizer
    {
    public:
        Customizer(const rime::Path &source_path,
                   const rime::Path &dest_path,
                   const string &version_key)
            : source_path_(source_path),
              dest_path_(dest_path),
              version_key_(version_key) {}

        // DEPRECATED: in favor of auto-patch config compiler plugin
        bool UpdateConfigFile();

    protected:
        rime::Path source_path_;
        rime::Path dest_path_;
        string version_key_;
    };

} // namespace rime

#endif // RIME_CUSTOMIZER_H_
