#pragma once

#include <Handle.h>

namespace Balau {

class Input : public SeekableHandle {
  public:
      Input(const char * fname) throw (GeneralException);
    virtual void close() throw (GeneralException);
    virtual bool isClosed();
    virtual bool canRead();
    virtual const char * getName();
    virtual off_t getSize();
    virtual time_t getMTime();
  private:
    int m_fd;
    String m_name;
    off_t m_size;
    time_t m_mtime;
};

};
