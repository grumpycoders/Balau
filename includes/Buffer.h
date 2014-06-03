#pragma once

#include <Handle.h>

namespace Balau {

class Buffer : public SeekableHandle {
  public:
      Buffer(const uint8_t * buffer, size_t s) : m_buffer(const_cast<uint8_t *>(buffer)), m_bufSize(s), m_fromConst(true) { }
      Buffer() throw (GeneralException) { }
      virtual ~Buffer() override;
    virtual void close() throw (GeneralException) override;
    virtual ssize_t read(void * buf, size_t count) throw (GeneralException) override;
    virtual ssize_t write(const void * buf, size_t count) throw (GeneralException) override;
    virtual bool isClosed() override;
    virtual bool isEOF() override;
    virtual bool canRead() override;
    virtual bool canWrite() override;
    virtual const char * getName() override;
    virtual off64_t getSize() override;
    const uint8_t * getBuffer() { return m_buffer + rtell(); }
    void reset();
    void clear();
    void rewind() { rseek(0); wseek(0); }
    void borrow(const uint8_t * buffer, size_t s);
  private:
    uint8_t * m_buffer = NULL;
    bool m_fromConst = false;
    off64_t m_bufSize = 0, m_numBlocks = 0;
};

};
