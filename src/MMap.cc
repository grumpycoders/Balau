#include <MMap.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

namespace Balau {

#ifdef _WIN32

class MMapPlatform {
  public:
    std::tuple<bool, const uint8_t *, size_t> open(String fname) {
        bool result = false;
        size_t s = 0;
        TCHAR * fnameTchar;
#ifdef UNICODE
        fname.do_iconv("UTF-8", "UNICODELITTLE");
        fnameTchar = (TCHAR *) alloca(fname.strlen() + 2);
        memset(fnameTchar, 0, fname.strlen() + 2);
        memcpy(fnameTchar, fname.to_charp(), fname.strlen());
#else
        fnameTchar = fname.to_charp();
#endif

        m_handle = CreateFile(fnameTchar, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

        if (m_handle == INVALID_HANDLE_VALUE)
            return std::tie(result, m_ptr, s);

        ScopedLambda hc([&]() { if (!result) CloseHandle(m_handle); m_handle = INVALID_HANDLE_VALUE; });

        LARGE_INTEGER fsize;
        if (!GetFileSizeEx(m_handle, (PLARGE_INTEGER) &fsize))
            return std::tie(result, m_ptr, s);

        s = fsize.QuadPart;

        m_mapObject = CreateFileMapping(m_handle, 0, PAGE_READONLY, fsize.HighPart, fsize.LowPart, NULL);

        if (m_mapObject == NULL)             // right, because getting NULL (which is 0) is better 
            return std::tie(result, m_ptr, s); // than getting the usual INVALID_HANDLE_VALUE (which is -1),
                                             // in order to make a sane and uniform API...
        {
            ScopedLambda mc([&]() { if (!result) CloseHandle(m_mapObject); m_mapObject = (HANDLE) NULL; });

            // and of course, CreateFileMapping takes the 64 bits size in high / low format,
            // whereas MapViewOfFile takes the 64 bits size in a single 64 bits size_t. Sure. Makes sense.
            m_ptr = (const uint8_t *)MapViewOfFile(m_mapObject, FILE_MAP_READ, 0, 0, s);

            result = !!m_ptr;
        }

        return std::tie(result, m_ptr, s);
    }
    void close() {
        if (m_ptr)
            UnmapViewOfFile(m_ptr);
        if (m_mapObject)
            CloseHandle(m_mapObject);
        if (m_handle != INVALID_HANDLE_VALUE)
            CloseHandle(m_handle);
        m_ptr = NULL;
        m_mapObject = (HANDLE) NULL;
        m_handle = INVALID_HANDLE_VALUE;
    }
    HANDLE m_handle = INVALID_HANDLE_VALUE;
    HANDLE m_mapObject = (HANDLE) NULL; // see comment above
    const uint8_t * m_ptr = NULL;
};

#else

class MMapPlatform {
  public:
    std::tuple<bool, const uint8_t *, size_t> open(const String & fname) {
        bool result = false;
        m_size = 0;
        m_fd = ::open(fname.to_charp(), O_RDONLY);

        if (m_fd < 0)
            return std::tie(result, m_ptr, m_size);

        ScopedLambda hc([&]() { if (!result) ::close(m_fd); m_fd = -1; });

        m_size = lseek(m_fd, 0, SEEK_END);
        lseek(m_fd, 0, SEEK_SET);

        if (m_size < 0)
            return std::tie(result, m_ptr, m_size);

        m_ptr = (uint8_t *) mmap(NULL, m_size, PROT_READ, MAP_PRIVATE, m_fd, 0);

        result = !!m_ptr;

        return std::tie(result, m_ptr, m_size);
    }
    void close() {
        if (m_ptr)
            munmap(m_ptr, m_size);
        if (m_fd >= 0)
            ::close(m_fd);
        m_ptr = NULL;
        m_fd = -1;
    }
    int m_fd = -1;
    uint8_t * m_ptr = NULL;
    ssize_t m_size;
};

#endif

};

Balau::MMap::MMap(const char * fname) {
    m_fname = fname;
    m_name.set("mmap of %s", m_fname.to_charp());
    m_platform = new MMapPlatform();
}

void Balau::MMap::open() throw (GeneralException) {
    bool result;
    const uint8_t * ptr;
    size_t s;
    std::tie(result, ptr, s) = m_platform->open(m_fname);
    if (!result)
        throw GeneralException("Unable to map file");

    borrow(ptr, s);
}

void Balau::MMap::close() throw (GeneralException) {
    m_platform->close();
    Buffer::close();
}

Balau::MMap::~MMap() {
    m_platform->close();
    delete m_platform;
}
