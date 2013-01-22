#pragma once

#include <Handle.h>

namespace Balau {

class Buffer : public SeekableHandle {
  public:
      Buffer(const uint8_t * buffer, size_t s) : m_buffer(const_cast<uint8_t *>(buffer)), m_bufSize(s), m_fromConst(true) { }
      Buffer() throw (GeneralException) { }
      virtual ~Buffer();
    virtual void close() throw (GeneralException);
    virtual ssize_t read(void * buf, size_t count) throw (GeneralException);
    virtual ssize_t write(const void * buf, size_t count) throw (GeneralException);
    virtual bool isClosed();
    virtual bool isEOF();
    virtual bool canRead();
    virtual bool canWrite();
    virtual const char * getName();
    virtual off_t getSize();
    const uint8_t * getBuffer() { return m_buffer + rtell(); }
    void reset();
    void rewind() { rseek(0); wseek(0); }
  private:
    uint8_t * m_buffer = NULL;
    bool m_fromConst = false;
    off_t m_bufSize = 0, m_numBlocks = 0;
};

};
