#pragma once

#include <BString.h>

namespace Balau {

class GeneralException {
  public:
      GeneralException(const char * msg) : m_msg(::strdup(msg)) { }
      GeneralException(const String & msg) : m_msg(msg.strdup()) { }
      GeneralException(const GeneralException & e) : m_msg(strdup(e.m_msg)) { }
      ~GeneralException() { if (m_msg) free(m_msg); }
    const char * getMsg() const { return m_msg; }

  protected:
      GeneralException() : m_msg(0) { }
    void setMsg(char * msg) { if (m_msg) free(m_msg); m_msg = msg; }
  private:
    char * m_msg;
};

static inline void AssertHelper(const String & msg) throw(GeneralException) { throw GeneralException(msg); }

};

#define Assert(c) if (!(c)) { \
    Balau::String msg; \
    msg.set("Assertion " #c " failed at %s:%i", __FILE__, __LINE__); \
    Balau::AssertHelper(msg); \
}
