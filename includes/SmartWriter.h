#pragma once

#include <Handle.h>

class SmartWriterTask;

namespace Balau {

class SmartWriter : public Filter {
  public:
      SmartWriter(IO<Handle> h) : Filter(h) { AAssert(h->canWrite(), "SmartWriter can't write"); m_name.set("SmartWriter(%s)", h->getName()); }
    virtual ssize_t write(const void * buf, size_t count) throw (GeneralException) override;
    virtual const char * getName() override { return m_name.to_charp(); }
    virtual void close() throw (GeneralException) override;
  private:
    SmartWriterTask * m_writerTask = NULL;
    String m_name;
};

}
