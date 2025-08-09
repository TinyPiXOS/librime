//
// Copyright RIME Developers
// Distributed under the BSD License
//
// register components
//
// 2011-06-30 GONG Chen <chen.sst@gmail.com>
//
#include <fstream>
// #include <boost/filesystem.hpp>
// #include <boost/interprocess/file_mapping.hpp>
// #include <boost/interprocess/mapped_region.hpp>
#include <rime/dict/mapped_file.h>

#include <rime/rimePath.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef BOOST_RESIZE_FILE

#define RESIZE_FILE boost::filesystem::resize_file

#else

#ifdef _WIN32
#include <windows.h>
#define RESIZE_FILE(P, SZ) (resize_file_api(P, SZ) != 0)
static BOOL resize_file_api(const char *p, boost::uintmax_t size)
{
    HANDLE handle = CreateFileA(p, GENERIC_WRITE, 0, 0, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, 0);
    LARGE_INTEGER sz;
    sz.QuadPart = size;
    return handle != INVALID_HANDLE_VALUE && ::SetFilePointerEx(handle, sz, 0, FILE_BEGIN) && ::SetEndOfFile(handle) && ::CloseHandle(handle);
}
#else
#include <unistd.h>
#define RESIZE_FILE(P, SZ) (::truncate(P, SZ) == 0)
#endif // _WIN32

#endif // BOOST_RESIZE_FILE

namespace rime
{
    class MappedFileImpl
    {
    public:
        enum OpenMode
        {
            kOpenReadOnly,
            kOpenReadWrite,
        };

        MappedFileImpl(const std::string &file_name, OpenMode mode)
        {
            open_posix(file_name, mode);
        }

        ~MappedFileImpl()
        {
            close_posix();
        }

        bool Flush()
        {
            return msync(address_, size_, MS_SYNC) == 0;
        }

        void *get_address() const
        {
            return address_;
        }

        size_t get_size() const
        {
            return size_;
        }

    private:
        void open_posix(const std::string &file_name, OpenMode mode)
        {
            // 打开文件
            int flags = (mode == kOpenReadOnly) ? O_RDONLY : O_RDWR;
            file_descriptor_ = open(file_name.c_str(), flags);
            if (file_descriptor_ == -1)
            {
                throw std::runtime_error("Cannot open file: " + file_name);
            }

            // 获取文件大小
            struct stat st;
            if (fstat(file_descriptor_, &st) == -1)
            {
                close(file_descriptor_);
                throw std::runtime_error("Cannot get file size");
            }
            size_ = static_cast<size_t>(st.st_size);

            // 映射文件
            int prot = (mode == kOpenReadOnly) ? PROT_READ : (PROT_READ | PROT_WRITE);
            address_ = mmap(nullptr, size_, prot, MAP_SHARED, file_descriptor_, 0);
            if (address_ == MAP_FAILED)
            {
                close(file_descriptor_);
                throw std::runtime_error("Cannot map file");
            }
        }

        void close_posix()
        {
            if (address_)
            {
                munmap(address_, size_);
                address_ = nullptr;
            }
            if (file_descriptor_ != -1)
            {
                close(file_descriptor_);
                file_descriptor_ = -1;
            }
        }

        int file_descriptor_ = -1;

        void *address_ = nullptr;
        size_t size_ = 0;
    };

#if 0
    class MappedFileImpl
    {
    public:
        enum OpenMode
        {
            kOpenReadOnly,
            kOpenReadWrite,
        };

        MappedFileImpl(const string &file_name, OpenMode mode)
        {
            boost::interprocess::mode_t file_mapping_mode =
                (mode == kOpenReadOnly) ? boost::interprocess::read_only
                                        : boost::interprocess::read_write;
            file_.reset(new boost::interprocess::file_mapping(file_name.c_str(), file_mapping_mode));
            region_.reset(new boost::interprocess::mapped_region(*file_, file_mapping_mode));
        }
        ~MappedFileImpl()
        {
            region_.reset();
            file_.reset();
        }
        bool Flush()
        {
            return region_->flush();
        }
        void *get_address() const
        {
            return region_->get_address();
        }
        size_t get_size() const
        {
            return region_->get_size();
        }

    private:
        the<boost::interprocess::file_mapping> file_;
        the<boost::interprocess::mapped_region> region_;
    };
#endif

    MappedFile::MappedFile(const string &file_name)
        : file_name_(file_name)
    {
    }

    MappedFile::~MappedFile()
    {
        if (file_)
        {
            file_.reset();
        }
    }

    bool MappedFile::Create(size_t capacity)
    {
        if (Exists())
        {
            LOG(INFO) << "overwriting file '" << file_name_ << "'.";
            Resize(capacity);
        }
        else
        {
            LOG(INFO) << "creating file '" << file_name_ << "'.";
            std::filebuf fbuf;
            fbuf.open(file_name_.c_str(),
                      std::ios_base::in | std::ios_base::out |
                          std::ios_base::trunc | std::ios_base::binary);
            if (capacity > 0)
            {
                fbuf.pubseekoff(capacity - 1, std::ios_base::beg);
                fbuf.sputc(0);
            }
            fbuf.close();
        }
        LOG(INFO) << "opening file for read/write access.";
        file_.reset(new MappedFileImpl(file_name_, MappedFileImpl::kOpenReadWrite));
        size_ = 0;
        return bool(file_);
    }

    bool MappedFile::OpenReadOnly()
    {
        if (!Exists())
        {
            LOG(ERROR) << "attempt to open non-existent file '" << file_name_ << "'.";
            return false;
        }
        file_.reset(new MappedFileImpl(file_name_, MappedFileImpl::kOpenReadOnly));
        size_ = file_->get_size();
        return bool(file_);
    }

    bool MappedFile::OpenReadWrite()
    {
        if (!Exists())
        {
            LOG(ERROR) << "attempt to open non-existent file '" << file_name_ << "'.";
            return false;
        }
        file_.reset(new MappedFileImpl(file_name_, MappedFileImpl::kOpenReadWrite));
        size_ = 0;
        return bool(file_);
    }

    void MappedFile::Close()
    {
        if (file_)
        {
            file_.reset();
            size_ = 0;
        }
    }

    bool MappedFile::Exists() const
    {
        // return boost::filesystem::exists(file_name_);
        return rime::Exists(file_name_);
    }

    bool MappedFile::IsOpen() const
    {
        return bool(file_);
    }

    bool MappedFile::Flush()
    {
        if (!file_)
            return false;
        return file_->Flush();
    }

    bool MappedFile::ShrinkToFit()
    {
        LOG(INFO) << "shrinking file to fit data size. capacity: " << capacity();
        return Resize(size_);
    }

    bool MappedFile::Remove()
    {
        if (IsOpen())
            Close();
        // return boost::interprocess::file_mapping::remove(file_name_.c_str());
        // return remove_file(file_name_.c_str());
        return rime::remove_file(file_name_);
    }

    bool MappedFile::Resize(size_t capacity)
    {
        LOG(INFO) << "resize file to: " << capacity;
        if (IsOpen())
            Close();
        try
        {
            RESIZE_FILE(file_name_.c_str(), capacity);
        }
        catch (...)
        {
            return false;
        }
        return true;
    }

    String *MappedFile::CreateString(const string &str)
    {
        String *ret = Allocate<String>();
        if (ret && !str.empty())
        {
            CopyString(str, ret);
        }
        return ret;
    }

    bool MappedFile::CopyString(const string &src, String *dest)
    {
        if (!dest)
            return false;
        size_t size = src.length() + 1;
        char *ptr = Allocate<char>(size);
        if (!ptr)
            return false;
        std::strncpy(ptr, src.c_str(), size);
        dest->data = ptr;
        return true;
    }

    size_t MappedFile::capacity() const
    {
        return file_->get_size();
    }

    char *MappedFile::address() const
    {
        return reinterpret_cast<char *>(file_->get_address());
    }

} // namespace rime
