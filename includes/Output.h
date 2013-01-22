#pragma once

#include <Handle.h>

namespace Balau {

class Output : public SeekableHandle {
  public:
      Output(const char * fname);
    void open(bool truncate = true) throw (GeneralException);
    virtual void close() throw (GeneralException);
    virtual ssize_t write(const void * buf, size_t count) throw (GeneralException);
    virtual bool isClosed();
    virtual bool canWrite();
    virtual const char * getName();
    virtual off_t getSize();
    virtual time_t getMTime();
    virtual bool isPendingComplete();
    const char * getFName() { return m_fname.to_charp(); }
  private:
    int m_fd = -1;
    String m_name;
    String m_fname;
    off_t m_size = -1;
    time_t m_mtime = -1;
    void * m_pendingOp = NULL;
};

};
