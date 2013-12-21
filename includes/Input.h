#pragma once

#include <Handle.h>

namespace Balau {

class Input : public SeekableHandle {
  public:
      Input(const char * fname);
      Input(const String & str) : Input(str.to_charp()) { }
    void open() throw (GeneralException);
    virtual void close() throw (GeneralException);
    virtual ssize_t read(void * buf, size_t count) throw (GeneralException);
    virtual bool isClosed();
    virtual bool canRead();
    virtual const char * getName();
    virtual off64_t getSize();
    virtual time_t getMTime();
    virtual bool isPendingComplete();
    const char * getFName() { return m_fname.to_charp(); }
  private:
    int m_fd = -1;
    String m_name;
    String m_fname;
    off64_t m_size = -1;
    time_t m_mtime = -1;
    void * m_pendingOp = NULL;
};

};
