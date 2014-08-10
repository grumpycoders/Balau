#include <memory>
#include <typeinfo>
#include <errno.h>
#include "ev++.h"
#include "Main.h"
#include "TaskMan.h"
#include "Handle.h"
#include "Printer.h"
#include "Async.h"

#ifdef _MSC_VER
#include <direct.h>
typedef int mode_t;
#endif

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
    ssize_t total = 0;
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
    ssize_t total = 0;
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

template<class T>
Balau::Future<T> genericRead(Balau::IO<Balau::Handle> t) {
    T b;
    size_t c = 0;
    return Balau::Future<T>([t, b, c]() mutable {
        do {
            ssize_t r = t->read(((uint8_t *) &b) + c, sizeof(T) - c);
            AAssert(r >= 0, "genericRead got an error: %zi", r);
            c += r;
        } while ((c < sizeof(T)) && !t->isEOF());
        return b;
    });
}

template<class T>
Balau::Future<T> genericReadBE(Balau::IO<Balau::Handle> t) {
    T b;
    size_t c = sizeof(T);
    return Balau::Future<T>([t, b, c]() mutable {
        do {
            ssize_t r = t->read(((uint8_t *) &b) + c - 1, 1);
            AAssert(r >= 0, "genericReadBE got an error: %zi", r);
            c -= r;
        } while (c && !t->isEOF());
        return b;
    });
}

Balau::Future<uint8_t>  Balau::Handle::readU8()  { return genericRead<uint8_t> (this); }
Balau::Future<int8_t>   Balau::Handle::readI8()  { return genericRead<int8_t>  (this); }

Balau::Future<uint16_t> Balau::Handle::readU16() {
    if (m_bigEndianMode)
        return readBEU16();
    else
        return readLEU16();
}

Balau::Future<uint32_t> Balau::Handle::readU32() {
    if (m_bigEndianMode)
        return readBEU32();
    else
        return readLEU32();
}

Balau::Future<uint64_t> Balau::Handle::readU64() {
    if (m_bigEndianMode)
        return readBEU64();
    else
        return readLEU64();
}

Balau::Future<int16_t>  Balau::Handle::readI16() {
    if (m_bigEndianMode)
        return readBEI16();
    else
        return readLEI16();
}

Balau::Future<int32_t>  Balau::Handle::readI32() {
    if (m_bigEndianMode)
        return readBEI32();
    else
        return readLEI32();
}

Balau::Future<int64_t>  Balau::Handle::readI64() {
    if (m_bigEndianMode)
        return readBEI64();
    else
        return readLEI64();
}

Balau::Future<uint16_t> Balau::Handle::readLEU16() { return genericRead<uint16_t>(this); }
Balau::Future<uint32_t> Balau::Handle::readLEU32() { return genericRead<uint32_t>(this); }
Balau::Future<uint64_t> Balau::Handle::readLEU64() { return genericRead<uint64_t>(this); }
Balau::Future<int16_t>  Balau::Handle::readLEI16() { return genericRead<int16_t> (this); }
Balau::Future<int32_t>  Balau::Handle::readLEI32() { return genericRead<int32_t> (this); }
Balau::Future<int64_t>  Balau::Handle::readLEI64() { return genericRead<int64_t> (this); }

Balau::Future<uint16_t> Balau::Handle::readBEU16() { return genericReadBE<uint16_t>(this); }
Balau::Future<uint32_t> Balau::Handle::readBEU32() { return genericReadBE<uint32_t>(this); }
Balau::Future<uint64_t> Balau::Handle::readBEU64() { return genericReadBE<uint64_t>(this); }
Balau::Future<int16_t>  Balau::Handle::readBEI16() { return genericReadBE<int16_t> (this); }
Balau::Future<int32_t>  Balau::Handle::readBEI32() { return genericReadBE<int32_t> (this); }
Balau::Future<int64_t>  Balau::Handle::readBEI64() { return genericReadBE<int64_t> (this); }

template<class T>
Balau::Future<void> genericWrite(Balau::IO<Balau::Handle> t, T b) {
    size_t c = 0;
    return Balau::Future<void>([t, b, c]() mutable {
        do {
            ssize_t r = t->write(((uint8_t *) &b) + c, sizeof(T) - c);
            c += r;
        } while (c < sizeof(T));
    });
}

template<class T>
Balau::Future<void> genericWriteBE(Balau::IO<Balau::Handle> t, T v) {
    size_t c = sizeof(T);
    return Balau::Future<void>([t, v, c]() mutable {
        do {
            ssize_t r = t->write(((uint8_t *) &v) + c - 1, 1);
            AAssert(r >= 0, "genericWriteBE got an error: %zi", r);
            c -= r;
        } while (c && !t->isEOF());
    });
}

Balau::Future<void> Balau::Handle::writeU8(uint8_t v) { return genericWrite<uint8_t>(this, v); }
Balau::Future<void> Balau::Handle::writeI8(int8_t  v) { return genericWrite<int8_t >(this, v); }

Balau::Future<void> Balau::Handle::writeU16(uint16_t v) {
    if (m_bigEndianMode)
        return writeBEU16(v);
    else
        return writeLEU16(v);
}

Balau::Future<void> Balau::Handle::writeU32(uint32_t v) {
    if (m_bigEndianMode)
        return writeBEU32(v);
    else
        return writeLEU32(v);
}

Balau::Future<void> Balau::Handle::writeU64(uint64_t v) {
    if (m_bigEndianMode)
        return writeBEU64(v);
    else
        return writeLEU64(v);
}

Balau::Future<void> Balau::Handle::writeI16(int16_t v) {
    if (m_bigEndianMode)
        return writeBEI16(v);
    else
        return writeLEI16(v);
}

Balau::Future<void> Balau::Handle::writeI32(int32_t v) {
    if (m_bigEndianMode)
        return writeBEI32(v);
    else
        return writeLEI32(v);
}

Balau::Future<void> Balau::Handle::writeI64(int64_t v) {
    if (m_bigEndianMode)
        return writeBEI64(v);
    else
        return writeLEI64(v);
}

Balau::Future<void> Balau::Handle::writeLEU16(uint16_t v) { return genericWrite<uint16_t>(this, v); }
Balau::Future<void> Balau::Handle::writeLEU32(uint32_t v) { return genericWrite<uint32_t>(this, v); }
Balau::Future<void> Balau::Handle::writeLEU64(uint64_t v) { return genericWrite<uint64_t>(this, v); }
Balau::Future<void> Balau::Handle::writeLEI16(int16_t  v) { return genericWrite<int16_t >(this, v); }
Balau::Future<void> Balau::Handle::writeLEI32(int32_t  v) { return genericWrite<int32_t >(this, v); }
Balau::Future<void> Balau::Handle::writeLEI64(int64_t  v) { return genericWrite<int64_t >(this, v); }

Balau::Future<void> Balau::Handle::writeBEU16(uint16_t v) { return genericWriteBE<uint16_t>(this, v); }
Balau::Future<void> Balau::Handle::writeBEU32(uint32_t v) { return genericWriteBE<uint32_t>(this, v); }
Balau::Future<void> Balau::Handle::writeBEU64(uint64_t v) { return genericWriteBE<uint64_t>(this, v); }
Balau::Future<void> Balau::Handle::writeBEI16(int16_t  v) { return genericWriteBE<int16_t >(this, v); }
Balau::Future<void> Balau::Handle::writeBEI32(int32_t  v) { return genericWriteBE<int32_t >(this, v); }
Balau::Future<void> Balau::Handle::writeBEI64(int64_t  v) { return genericWriteBE<int64_t >(this, v); }

void Balau::Handle::rseek(off64_t offset, int whence) throw (GeneralException) {
    if (canSeek())
        throw GeneralException(String("Handle ") + getName() + " can seek, but rseek() not implemented (missing in class " + ClassName(this).c_str() + ")");
    else
        throw GeneralException("Handle can't seek");
}

off64_t Balau::Handle::rtell() throw (GeneralException) {
    if (canSeek())
        throw GeneralException(String("Handle ") + getName() + " can seek, but rtell() not implemented (missing in class " + ClassName(this).c_str() + ")");
    else
        throw GeneralException("Handle can't seek");
}

void Balau::SeekableHandle::rseek(off64_t offset, int whence) throw (GeneralException) {
    AAssert(canRead() || canWrite(), "Can't use a SeekableHandle with a Handle that can neither read or write...");
    off64_t size;
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

void Balau::SeekableHandle::wseek(off64_t offset, int whence) throw (GeneralException) {
    AAssert(canRead() || canWrite(), "Can't use a SeekableHandle with a Handle that can neither read or write...");
    off64_t size;
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

off64_t Balau::SeekableHandle::rtell() throw (GeneralException) {
    AAssert(canRead() || canWrite(), "Can't use a SeekableHandle with a Handle that can neither read or write...");
    if (!canRead())
        return wtell();
    return m_rOffset;
}

off64_t Balau::SeekableHandle::wtell() throw (GeneralException) {
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
#ifdef _MSC_VER
        int r = m_results->result = mkdir(m_path);
#else
        int r = m_results->result = mkdir(m_path, m_mode);
#endif
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
