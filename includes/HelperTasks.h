#pragma once

#include <Task.h>
#include <StacklessTask.h>
#include <Handle.h>

namespace Balau {

#define COPY_BUFSIZE 4096

class CopyTask : public StacklessTask {
  public:
      CopyTask(IO<Handle> s, IO<Handle> d, ssize_t tocopy = -1);
    virtual const char * getName() const override { return m_name.to_charp(); }
    virtual void Do();
  private:
    char m_buffer[COPY_BUFSIZE];
    IO<Handle> m_s, m_d;
    ssize_t m_tocopy, m_current = 0, m_written, m_read;
    size_t m_towrite;
    String m_name;
};

};
