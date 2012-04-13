#pragma once

#include <Exceptions.h>
#include <Printer.h>
#include <BString.h>
#include <Atomic.h>

namespace Balau {

class FileSystem {
  public:
    static int mkdir(const char * path) throw (GeneralException);
};

class ENoEnt : public GeneralException {
  public:
      ENoEnt(const char * name) : GeneralException(String("No such file or directory: `") + name + "'") { }
};

class IOBase;

template<class T>
class IO;

namespace Events {

class BaseEvent;

};

class Handle {
  public:
      virtual ~Handle() { AAssert(m_refCount == 0, "Do not use handles directly; warp them in IO<>"); }
    virtual void close() throw (GeneralException) = 0;
    virtual bool isClosed() = 0;
    virtual bool isEOF() = 0;
    virtual bool canSeek();
    virtual bool canRead();
    virtual bool canWrite();
    virtual const char * getName() = 0;
    virtual ssize_t read(void * buf, size_t count) throw (GeneralException);
    virtual ssize_t write(const void * buf, size_t count) throw (GeneralException);
    template <size_t L> void writeString(const char (&str)[L]) { writeString(str, L - 1); }
    void writeString(const char * str, ssize_t len) { if (len < 0) len = strlen(str); forceWrite(str, len); }
    void writeString(const String & str) { forceWrite(str.to_charp(), str.strlen()); }
    void seek(off_t offset, int whence = SEEK_SET) { rseek(offset, whence); }
    virtual void rseek(off_t offset, int whence = SEEK_SET) throw (GeneralException);
    virtual void wseek(off_t offset, int whence = SEEK_SET) throw (GeneralException);
    off_t tell() { return rtell(); }
    virtual off_t rtell() throw (GeneralException);
    virtual off_t wtell() throw (GeneralException);
    virtual off_t getSize();
    virtual time_t getMTime();
    ssize_t forceRead(void * buf, size_t count, Events::BaseEvent * evt = NULL) throw (GeneralException);
    ssize_t forceWrite(const void * buf, size_t count, Events::BaseEvent * evt = NULL) throw (GeneralException);
    ssize_t write(const String & str) { return write(str.to_charp(), str.strlen()); }
    ssize_t forceWrite(const String & str) { return forceWrite(str.to_charp(), str.strlen()); }
    uint8_t readU8() { uint8_t r; read(&r, 1); return r; }
  protected:
      Handle() : m_refCount(0) { }
  private:
    void addRef() { Atomic::Increment(&m_refCount); }
    void delRef() {
        if (Atomic::Decrement(&m_refCount) == 0) {
            if (!isClosed())
                close();
            delete this;
        }
    }
    friend class IOBase;
    template<class T>
    friend class IO;

    volatile int m_refCount;

      Handle(const Handle &) = delete;
    Handle & operator=(const Handle &) = delete;
};

class HPrinter : public Handle {
  public:
    virtual void close() throw (GeneralException) { }
    virtual bool isClosed() { return false; }
    virtual bool isEOF() { return false; }
    virtual bool canWrite() { return true; }
    virtual const char * getName() { return "HPrinter"; }
    virtual ssize_t write(const void * buf, size_t count) throw (GeneralException) {
        Printer::print("%s", (const char *) buf);
        return count;
    }
};

class IOBase {
  public:
      IOBase() : m_h(NULL) { }
      ~IOBase() { if (m_h) m_h->delRef(); }
  protected:
    void setHandle(Handle * h) { m_h = h; if (m_h) m_h->addRef(); }
    Handle * m_h;
    template<class T>
    friend class IO;
};

template<class T>
class IO : public IOBase {
  public:
      IO() { }
      IO(T * h) { setHandle(h); }
      IO(const IO<T> & io) { if (io.m_h) setHandle(io.m_h); }
      template<class U>
      IO(const IO<U> & io) { if (io.m_h) setHandle(io.m_h); }
      template<class U>
      bool isA() { return !!dynamic_cast<U *>(m_h); }
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
    virtual void rseek(off_t offset, int whence = SEEK_SET) throw (GeneralException);
    virtual void wseek(off_t offset, int whence = SEEK_SET) throw (GeneralException);
    virtual off_t rtell() throw (GeneralException);
    virtual off_t wtell() throw (GeneralException);
    virtual bool isEOF();
  protected:
    off_t getWOffset() { return m_wOffset; }
    off_t getROffset() { return m_rOffset; }
  private:
    off_t m_wOffset, m_rOffset;
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
    virtual void rseek(off_t offset, int whence = SEEK_SET) throw (GeneralException) { m_io->rseek(offset, whence); }
    virtual void wseek(off_t offset, int whence = SEEK_SET) throw (GeneralException) { throw GeneralException("Can't write"); }
    virtual off_t rtell() throw (GeneralException) { return m_io->rtell(); }
    virtual off_t wtell() throw (GeneralException) { throw GeneralException("Can't write"); }
    virtual off_t getSize() { return m_io->getSize(); }
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
    virtual void rseek(off_t offset, int whence = SEEK_SET) throw (GeneralException) { throw GeneralException("Can't read"); }
    virtual void wseek(off_t offset, int whence = SEEK_SET) throw (GeneralException) { return m_io->wseek(offset, whence); }
    virtual off_t rtell() throw (GeneralException) { throw GeneralException("Can't read"); }
    virtual off_t wtell() throw (GeneralException) { return m_io->wtell(); }
    virtual off_t getSize() { return m_io->getSize(); }
    virtual time_t getMTime() { return m_io->getMTime(); }
  private:
    IO<Handle> m_io;
};

};
