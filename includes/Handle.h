#pragma once

#include <Exceptions.h>
#include <Printer.h>

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

class Handle {
  public:
      virtual ~Handle() { Assert(m_refCount == 0); }
    virtual void close() throw (GeneralException) = 0;
    virtual bool isClosed() = 0;
    virtual bool isEOF() = 0;
    virtual bool canSeek();
    virtual bool canRead();
    virtual bool canWrite();
    virtual const char * getName() = 0;
    virtual ssize_t read(void * buf, size_t count) throw (GeneralException);
    virtual ssize_t write(const void * buf, size_t count) throw (GeneralException);
    void writeString(const char * str, size_t len = -1) { if (len < 0) len = strlen(str); write(str, len); }
    void writeString(const String & str) { write(str.to_charp(), str.strlen()); }
    virtual void rseek(off_t offset, int whence = SEEK_SET) throw (GeneralException);
    virtual void wseek(off_t offset, int whence = SEEK_SET) throw (GeneralException);
    virtual off_t rtell() throw (GeneralException);
    virtual off_t wtell() throw (GeneralException);
    virtual off_t getSize();
    virtual time_t getMTime();
  protected:
      Handle() : m_refCount(0) { }
  private:
    void addRef() { m_refCount++; }
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

    int m_refCount;
};

class IOBase {
  public:
      IOBase() : m_h(NULL) { }
      ~IOBase() { if (m_h) m_h->delRef(); }
  protected:
    void setHandle(Handle * h) { m_h = h; m_h->addRef(); }
    Handle * m_h;
    template<class T>
    friend class IO;
};

template<class T>
class IO : public IOBase {
  public:
      IO() { }
      IO(T * h) { setHandle(h); }
      IO(const IO<T> & io) { setHandle(io.m_h); }
      template<class U>
      IO(const IO<U> & io) { setHandle(io.m_h); }
    IO<T> & operator=(const IO<T> & io) { if (m_h) m_h->delRef(); setHandle(io.m_h); return *this; }
    T * operator->() { Assert(m_h); return dynamic_cast<T *>(m_h); }
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

};
