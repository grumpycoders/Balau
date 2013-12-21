#pragma once

#include <atomic>
#include <Task.h>
#include <Exceptions.h>
#include <Printer.h>
#include <BString.h>

#ifdef _MSC_VER
typedef __int64 off64_t;
#endif

namespace Balau {

class FileSystem {
  public:
    static int mkdir(const char * path) throw (GeneralException);
};

class ENoEnt : public GeneralException {
  public:
      ENoEnt(const char * name) : GeneralException(String("No such file or directory: `") + name + "'") { }
      ENoEnt(const String & name) : ENoEnt(name.to_charp()) { }
};

class IOBase;

template<class T>
class IO;

namespace Events {

class BaseEvent;

};

#ifdef _MSC_VER
#define WARN_UNUSED_RESULT
#else
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#endif

class Handle {
  public:
      virtual ~Handle() { AAssert(m_refCount == 0, "Do not use handles directly; warp them in IO<>"); }

    // things to implement when derivating
    virtual void close() throw (GeneralException) = 0;
    virtual bool isClosed() = 0;
    virtual bool isEOF() = 0;
    virtual const char * getName() = 0;

    // normal API
    virtual bool canSeek();
    virtual bool canRead();
    virtual bool canWrite();
    virtual ssize_t read(void * buf, size_t count) throw (GeneralException) WARN_UNUSED_RESULT;
    virtual ssize_t write(const void * buf, size_t count) throw (GeneralException) WARN_UNUSED_RESULT;
    virtual void rseek(off64_t offset, int whence = SEEK_SET) throw (GeneralException);
    virtual void wseek(off64_t offset, int whence = SEEK_SET) throw (GeneralException);
    virtual off64_t rtell() throw (GeneralException);
    virtual off64_t wtell() throw (GeneralException);
    virtual off64_t getSize();
    virtual time_t getMTime();
    virtual bool isPendingComplete() { return true; }

    // helpers
    off64_t tell() { return rtell(); }
    void seek(off64_t offset, int whence = SEEK_SET) { rseek(offset, whence); }

    Future<uint8_t>  readU8();
    Future<uint16_t> readU16();
    Future<uint32_t> readU32();
    Future<uint64_t> readU64();
    Future<int8_t>   readI8();
    Future<int16_t>  readI16();
    Future<int32_t>  readI32();
    Future<int64_t>  readI64();
	Future<uint8_t>  readBEU8();
	Future<uint16_t> readBEU16();
	Future<uint32_t> readBEU32();
	Future<uint64_t> readBEU64();
	Future<int8_t>   readBEI8();
	Future<int16_t>  readBEI16();
	Future<int32_t>  readBEI32();
	Future<int64_t>  readBEI64();
	Future<void>     writeU8(uint8_t);
    Future<void>     writeU16(uint16_t);
    Future<void>     writeU32(uint32_t);
    Future<void>     writeU64(uint64_t);
    Future<void>     writeI8 (int8_t);
    Future<void>     writeI16(int16_t);
    Future<void>     writeI32(int32_t);
    Future<void>     writeI64(int64_t);

    // these need to be changed into Future<>s
    template <size_t L>
    ssize_t writeString(const char (&str)[L]) WARN_UNUSED_RESULT;
    ssize_t writeString(const String & str) WARN_UNUSED_RESULT { return forceWrite(str.to_charp(), str.strlen()); }
    ssize_t writeString(const char * str, ssize_t len) WARN_UNUSED_RESULT { return forceWrite(str, len); }
    ssize_t forceRead(void * buf, size_t count, Events::BaseEvent * evt = NULL) throw (GeneralException) WARN_UNUSED_RESULT;
    ssize_t forceWrite(const void * buf, size_t count, Events::BaseEvent * evt = NULL) throw (GeneralException) WARN_UNUSED_RESULT;

  protected:
      Handle() : m_refCount(0) { }

  private:
    // the IO<> refcounting mechanism
    void addRef() { ++m_refCount; }
    void delRef() {
        if (--m_refCount == 0) {
            if (!isClosed())
                close();
            delete this;
        }
    }
    friend class IOBase;
    template<class T>
    friend class IO;

    std::atomic<int> m_refCount;

      Handle(const Handle &) = delete;
    Handle & operator=(const Handle &) = delete;
};

template <size_t L>
ssize_t Handle::writeString(const char (&str)[L]) { return writeString(str, L - 1); }

class HPrinter : public Handle {
  public:
    virtual void close() throw (GeneralException) { }
    virtual bool isClosed() { return false; }
    virtual bool isEOF() { return false; }
    virtual bool canWrite() { return true; }
    virtual const char * getName() { return "HPrinter"; }
    virtual ssize_t write(const void * buf, size_t count) throw (GeneralException) {
        Printer::print("%*s", (int) count, (const char *) buf);
        return count;
    }
};

class IOBase {
  private:
      IOBase() { }
      ~IOBase() { if (m_h) m_h->delRef(); }
    void setHandle(Handle * h) { m_h = h; if (m_h) m_h->addRef(); }
    Handle * m_h = NULL;
    template<class T>
    friend class IO;
};

template<class T>
class IO : public IOBase {
  public:
      IO() { }
      IO(T * h) { setHandle(h); }
      IO(const IO<T> & io) { if (io.m_h) setHandle(io.m_h); }
      IO(IO<T> && io) { m_h = io.m_h; io.m_h = NULL; }
      template<class U>
      IO(const IO<U> & io) { if (io.m_h) setHandle(io.m_h); }
      template<class U>
      IO(IO<U> && io) { m_h = io.m_h; io.m_h = NULL; }
    template<class U>
    bool isA() { return !!dynamic_cast<U *>(m_h); }
    template<class U>
    IO<U> asA() { IO<U> h(dynamic_cast<U *>(m_h)); return h; }
    IO<T> & operator=(const IO<T> & io) { if (m_h) m_h->delRef(); setHandle(io.m_h); return *this; }
    T * operator->() {
        AAssert(m_h, "Can't use %s->() with a null Handle", ClassName(this).c_str());
        T * r = dynamic_cast<T *>(m_h);
        AAssert(r, "%s->() used with an incompatible Handle type", ClassName(this).c_str());
        return r;
    }
    bool isNull() { return dynamic_cast<T *>(m_h); }
};

class SeekableHandle : public Handle {
  public:
      SeekableHandle() : m_wOffset(0), m_rOffset(0) { }
    virtual bool canSeek();
    virtual void rseek(off64_t offset, int whence = SEEK_SET) throw (GeneralException);
    virtual void wseek(off64_t offset, int whence = SEEK_SET) throw (GeneralException);
    virtual off64_t rtell() throw (GeneralException);
    virtual off64_t wtell() throw (GeneralException);
    virtual bool isEOF();
  protected:
    off64_t getWOffset() { return m_wOffset; }
    off64_t getROffset() { return m_rOffset; }
  private:
    off64_t m_wOffset, m_rOffset;
};

class ReadOnly : public Handle {
  public:
      ReadOnly(IO<Handle> & io) : m_io(io) { AAssert(m_io->canRead(), "You need to use ReadOnly with a Handle that can at least read"); }
    virtual void close() throw (GeneralException) { m_io->close(); }
    virtual bool isClosed() { return m_io->isClosed(); }
    virtual bool isEOF() { return m_io->isEOF(); }
    virtual bool canSeek() { return m_io->canSeek(); }
    virtual bool canRead() { return true; }
    virtual bool canWrite() { return false; }
    virtual const char * getName() { return m_io->getName(); }
    virtual ssize_t read(void * buf, size_t count) throw (GeneralException) { return m_io->read(buf, count); }
    virtual ssize_t write(const void * buf, size_t count) throw (GeneralException) { throw GeneralException("Can't write"); }
    virtual void rseek(off64_t offset, int whence = SEEK_SET) throw (GeneralException) { m_io->rseek(offset, whence); }
    virtual void wseek(off64_t offset, int whence = SEEK_SET) throw (GeneralException) { throw GeneralException("Can't write"); }
    virtual off64_t rtell() throw (GeneralException) { return m_io->rtell(); }
    virtual off64_t wtell() throw (GeneralException) { throw GeneralException("Can't write"); }
    virtual off64_t getSize() { return m_io->getSize(); }
    virtual time_t getMTime() { return m_io->getMTime(); }
  private:
    IO<Handle> m_io;
};

class WriteOnly : public Handle {
  public:
      WriteOnly(IO<Handle> & io) : m_io(io) { AAssert(m_io->canWrite(), "You need to use WriteOnly with a Handle that can at least write"); }
    virtual void close() throw (GeneralException) { m_io->close(); }
    virtual bool isClosed() { return m_io->isClosed(); }
    virtual bool isEOF() { return m_io->isEOF(); }
    virtual bool canSeek() { return m_io->canSeek(); }
    virtual bool canRead() { return false; }
    virtual bool canWrite() { return true; }
    virtual const char * getName() { return m_io->getName(); }
    virtual ssize_t read(void * buf, size_t count) throw (GeneralException) { throw GeneralException("Can't read"); }
    virtual ssize_t write(const void * buf, size_t count) throw (GeneralException) { return m_io->write(buf, count); }
    virtual void rseek(off64_t offset, int whence = SEEK_SET) throw (GeneralException) { throw GeneralException("Can't read"); }
    virtual void wseek(off64_t offset, int whence = SEEK_SET) throw (GeneralException) { return m_io->wseek(offset, whence); }
    virtual off64_t rtell() throw (GeneralException) { throw GeneralException("Can't read"); }
    virtual off64_t wtell() throw (GeneralException) { return m_io->wtell(); }
    virtual off64_t getSize() { return m_io->getSize(); }
    virtual time_t getMTime() { return m_io->getMTime(); }
  private:
    IO<Handle> m_io;
};

};
