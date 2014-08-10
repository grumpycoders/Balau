#pragma once

#include <Handle.h>

namespace Balau {

class BStream : public Filter {
  public:
      BStream(IO<Handle> h);
    virtual void close() throw (GeneralException) override;
    virtual bool isEOF() override { return (m_availBytes == 0) && Filter::isEOF(); }
    virtual const char * getName() override { return m_name.to_charp(); }
    virtual ssize_t read(void * buf, size_t count) throw (GeneralException);
    int peekNextByte();
    String readString(bool putNL = false);
    bool isEmpty() { return m_availBytes == 0; }
  private:
    uint8_t * m_buffer = NULL;
    size_t m_availBytes = 0;
    size_t m_cursor = 0;
    String m_name;
    bool m_passThru = false;
};

};
