#pragma once

#include <Exceptions.h>

namespace Balau {

class ENoEnt : public GeneralException {
  public:
      ENoEnt(const char * name) : GeneralException(String("No such file or directory: `") + name + "'") { }
};

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
    void delRef() { if (--m_refCount == 0) { if (!isClosed()) close(); delete this; } }
    int refCount() { return m_refCount; }
    friend class IO;

    int m_refCount;
};

class IO {
  public:
      IO() : m_h(NULL) { }
      IO(Handle * h) { setHandle(h); }
      ~IO() { if (m_h) m_h->delRef(); }
      IO(const IO & io) { setHandle(io.m_h); }
    IO & operator=(const IO & io) { if (m_h) m_h->delRef(); setHandle(io.m_h); return *this; }
    Handle * operator->() { Assert(m_h); return m_h; }
  protected:
    void setHandle(Handle * h) { m_h = h; m_h->addRef(); }
  private:
    Handle * m_h;
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
