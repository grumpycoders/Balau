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
    virtual bool canWrite() { return m_h->canWrite(); }
    virtual const char * getName();
    virtual ssize_t read(void * buf, size_t count) throw (GeneralException);
    virtual ssize_t write(const void * buf, size_t count) throw (GeneralException) { return m_h->write(buf, count); }
        virtual off64_t getSize();
    int peekNextByte();
    String readString(bool putNL = false);
    bool isEmpty() { return m_availBytes == 0; }
    void detach() { m_detached = true; }
  private:
    IO<Handle> m_h;
    uint8_t * m_buffer = NULL;
    size_t m_availBytes = 0;
    size_t m_cursor = 0;
    String m_name;
    bool m_passThru = false;
    bool m_detached = false;
    bool m_closed = false;
};

};
