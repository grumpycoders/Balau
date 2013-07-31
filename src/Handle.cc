#include <typeinfo>
#include <errno.h>
#include "ev++.h"
#include "Main.h"
#include "TaskMan.h"
#include "Handle.h"
#include "Printer.h"
#include "Async.h"

#ifdef _WIN32
static const char * strerror_r(int errorno, char * buf, size_t bufsize) {
#ifdef _MSVC
    strerror_s(buf, bufsize, errorno);
    return buf;
#else
    return strerror(errorno);
#endif
}
#endif

bool Balau::Handle::canSeek() { return false; }
bool Balau::Handle::canRead() { return false; }
bool Balau::Handle::canWrite() { return false; }
off_t Balau::Handle::getSize() { return -1; }
time_t Balau::Handle::getMTime() { return -1; }

ssize_t Balau::Handle::read(void * buf, size_t count) throw (GeneralException) {
    if (canRead())
        throw GeneralException(String("Handle ") + getName() + " can read, but read() not implemented (missing in class " + ClassName(this).c_str() + ")");
    else
        throw GeneralException("Handle can't read");
    return -1;
}

ssize_t Balau::Handle::write(const void * buf, size_t count) throw (GeneralException) {
    if (canWrite())
        throw GeneralException(String("Handle ") + getName() + " can write, but write() not implemented (missing in class " + ClassName(this).c_str() + ")");
    else
        throw GeneralException("Handle can't write");
    return -1;
}

ssize_t Balau::Handle::forceRead(void * _buf, size_t count, Events::BaseEvent * evt) throw (GeneralException) {
    ssize_t total;
    uint8_t * buf = (uint8_t *) _buf;
    if (!canRead())
        throw GeneralException("Handle can't read");

    while (count && !isClosed()) {
        ssize_t r;
        try {
            r = read(buf, count);
        }
        catch (EAgain & e) {
            if (evt && evt->gotSignal())
                return total;
            Task::operationYield(e.getEvent());
            continue;
        }
        if (r < 0)
            return r;
        total += r;
        count -= r;
        buf += r;
    }

    return total;
}

ssize_t Balau::Handle::forceWrite(const void * _buf, size_t count, Events::BaseEvent * evt) throw (GeneralException) {
    ssize_t total;
    const uint8_t * buf = (const uint8_t *) _buf;
    if (!canWrite())
        throw GeneralException("Handle can't write");

    while (count && !isClosed()) {
        ssize_t r;
        try {
            r = write(buf, count);
        }
        catch (EAgain & e) {
            if (evt && evt->gotSignal())
                return total;
            Task::operationYield(e.getEvent());
            continue;
        }
        if (r < 0)
            return r;
        total += r;
        count -= r;
        buf += r;
    }

    return total;
}

void Balau::Handle::rseek(off_t offset, int whence) throw (GeneralException) {
    if (canSeek())
        throw GeneralException(String("Handle ") + getName() + " can seek, but rseek() not implemented (missing in class " + ClassName(this).c_str() + ")");
    else
        throw GeneralException("Handle can't seek");
}

void Balau::Handle::wseek(off_t offset, int whence) throw (GeneralException) {
    rseek(offset, whence);
}

off_t Balau::Handle::rtell() throw (GeneralException) {
    if (canSeek())
        throw GeneralException(String("Handle ") + getName() + " can seek, but rtell() not implemented (missing in class " + ClassName(this).c_str() + ")");
    else
        throw GeneralException("Handle can't seek");
}

off_t Balau::Handle::wtell() throw (GeneralException) {
    return rtell();
}

bool Balau::SeekableHandle::canSeek() { return true; }

void Balau::SeekableHandle::rseek(off_t offset, int whence) throw (GeneralException) {
    AAssert(canRead() || canWrite(), "Can't use a SeekableHandle with a Handle that can neither read or write...");
    off_t size;
    if (!canRead())
        wseek(offset, whence);
    switch (whence) {
    case SEEK_SET:
        m_rOffset = offset;
        break;
    case SEEK_CUR:
        m_rOffset += offset;
        break;
    case SEEK_END:
        size = getSize();
        if (getSize() < 0)
            throw GeneralException("Can't seek from end in a Handle you don't know the max size");
        m_rOffset = size + offset;
        break;
    }
    if (m_rOffset < 0)
        m_rOffset = 0;
}

void Balau::SeekableHandle::wseek(off_t offset, int whence) throw (GeneralException) {
    AAssert(canRead() || canWrite(), "Can't use a SeekableHandle with a Handle that can neither read or write...");
    off_t size;
    if (!canWrite())
        rseek(offset, whence);
    switch (whence) {
    case SEEK_SET:
        m_wOffset = offset;
        break;
    case SEEK_CUR:
        m_wOffset += offset;
        break;
    case SEEK_END:
        size = getSize();
        if (getSize() < 0)
            throw GeneralException("Can't seek from end in a Handle you don't know the max size");
        m_wOffset = size + offset;
        break;
    }
    if (m_wOffset < 0)
        m_wOffset = 0;
}

off_t Balau::SeekableHandle::rtell() throw (GeneralException) {
    AAssert(canRead() || canWrite(), "Can't use a SeekableHandle with a Handle that can neither read or write...");
    if (!canRead())
        return wtell();
    return m_rOffset;
}

off_t Balau::SeekableHandle::wtell() throw (GeneralException) {
    AAssert(canRead() || canWrite(), "Can't use a SeekableHandle with a Handle that can neither read or write...");
    if (!canWrite())
        return rtell();
    return m_wOffset;
}

bool Balau::SeekableHandle::isEOF() {
    return m_rOffset == getSize();
}

namespace {

struct cbResults_t {
    Balau::Events::Custom evt;
    int result, errorno;
};

class AsyncOpMkdir : public Balau::AsyncOperation {
  public:
      AsyncOpMkdir(const char * path, mode_t mode, cbResults_t * results) : m_path(path), m_mode(mode), m_results(results) { }
    virtual void run() {
        int r = m_results->result = mkdir(m_path, m_mode);
        m_results->errorno = r < 0 ? errno : 0;
    }
    virtual void done() {
        m_results->evt.doSignal();
        delete this;
    }
  private:
    const char * m_path;
    mode_t m_mode;
    cbResults_t * m_results;
};

};

int Balau::FileSystem::mkdir(const char * path) throw (GeneralException) {
    cbResults_t cbResults;
    createAsyncOp(new AsyncOpMkdir(path, 0755, &cbResults));
    Task::operationYield(&cbResults.evt);

    if (cbResults.result < 0) {
        char str[4096];
        throw GeneralException(String("Unable to create directory ") + path + ": " + strerror_r(cbResults.errorno, str, sizeof(str)) + " (err#" + cbResults.errorno + ")");
    }

    return cbResults.result;
}
