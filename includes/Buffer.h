#pragma once

#include <Handle.h>

namespace Balau {

class Buffer : public SeekableHandle {
  public:
      Buffer() throw (GeneralException) : m_buffer(NULL), m_bufSize(0), m_numBlocks(0) { }
    virtual void close() throw (GeneralException);
    virtual ssize_t read(void * buf, size_t count) throw (GeneralException);
    virtual ssize_t write(const void * buf, size_t count) throw (GeneralException);
    virtual bool isClosed();
    virtual bool isEOF();
    virtual bool canRead();
    virtual bool canWrite();
    virtual const char * getName();
    virtual off_t getSize();
    void reset();
  private:
    uint8_t * m_buffer;
    off_t m_bufSize, m_numBlocks;
};

};
