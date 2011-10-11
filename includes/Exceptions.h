#pragma once

#include <cxxabi.h>
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

class ClassName {
  public:
      template<typename T> ClassName(T * ptr);
      ~ClassName() { free(m_demangled); }
    const char * c_str() const { return m_demangled; }
  private:
    char * m_demangled;
};

template<typename T>
ClassName::ClassName(T * ptr) {
    int status;
    m_demangled = abi::__cxa_demangle(typeid(*ptr).name(), 0, 0, &status);
}

};

#define Assert(c) if (!(c)) { \
    Balau::String msg; \
    msg.set("Assertion " #c " failed at %s:%i", __FILE__, __LINE__); \
    Balau::AssertHelper(msg); \
}
