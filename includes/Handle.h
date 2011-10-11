#pragma once

#include <Exceptions.h>

namespace Balau {

class EAgain : public GeneralException {
  public:
      EAgain() : GeneralException("Try Again") { }
};

class IO;

class Handle {
  public:
      virtual ~Handle() { Assert(m_refCount == 0); }
    virtual void close() throw (GeneralException) = 0;
    virtual bool isClosed() = 0;
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
      template<class T> IO(T * h) { setHandle(h); }
      ~IO() { m_h->delRef(); }
      IO(const IO & io) { setHandle(io.m_h); }
    Handle * operator->() { return m_h; }
  protected:
    void setHandle(Handle * h) { m_h = h; m_h->addRef(); }
  private:
    Handle * m_h;
};

class SeekableHandle : public Handle {
  public:
    virtual bool canSeek();
    virtual void rseek(off_t offset, int whence = SEEK_SET) throw (GeneralException);
    virtual void wseek(off_t offset, int whence = SEEK_SET) throw (GeneralException);
    virtual off_t rtell() throw (GeneralException);
    virtual off_t wtell() throw (GeneralException);
  private:
    off_t m_wOffset, m_rOffset;
};

};
