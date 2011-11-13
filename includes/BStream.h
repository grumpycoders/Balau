#pragma once

#include <Handle.h>

namespace Balau {

class BStream : public Handle {
  public:
      BStream(const IO<Handle> & h);
    virtual void close() throw (GeneralException);
    virtual bool isClosed();
    virtual bool isEOF();
    virtual bool canRead();
    virtual const char * getName();
    virtual ssize_t read(void * buf, size_t count) throw (GeneralException);
    virtual off_t getSize();
    int peekNextByte();
    String readString(bool putNL = false);
  private:
    IO<Handle> m_h;
    uint8_t * m_buffer;
    size_t m_availBytes;
    size_t m_cursor;
    String m_name;
};

};
