#pragma once

#include <Handle.h>

namespace Balau {

class Input : public SeekableHandle {
  public:
      Input(const char * fname) throw (GeneralException);
    virtual void close() throw (GeneralException);
    virtual bool isClosed();
    virtual const char * getName();
  private:
    int m_fd;
    String m_name;
};

};
