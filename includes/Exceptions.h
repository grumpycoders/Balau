#pragma once

#include <stdarg.h>
#include <typeinfo>
#include <cxxabi.h>
#include <BString.h>
#include <vector>

namespace Balau {

class ClassName {
  public:
      template<typename T> ClassName(T * ptr);
      ~ClassName() { free(m_demangled); }
    const char * c_str() const { return m_demangled; }
  private:
    char * m_demangled;
};

class GeneralException {
  public:
      GeneralException(const char * msg, const char * details = NULL, bool notrace = false) : m_msg(msg ? ::strdup(msg) : NULL) { setDetails(details); if (!notrace) genTrace(); }
      GeneralException(const String & msg, const char * details = NULL, bool notrace = false) : m_msg(msg.strdup()) { setDetails(details); if (!notrace) genTrace(); }
      GeneralException(const GeneralException & e) : m_msg(e.m_msg ? strdup(e.m_msg) : NULL), m_trace(e.m_trace) { setDetails(e.m_details); }
      GeneralException(GeneralException && e) : m_msg(e.m_msg), m_trace(e.m_trace), m_details(e.m_details) { e.m_msg = e.m_details = NULL; }
      // don't hate me, this is to generate typeinfo in the getMsg() case where there's no message.
      virtual ~GeneralException() { if (m_msg) free(m_msg); if (m_details) free(m_details); }
    const char * getMsg() const {
        if (!m_msg)
            m_msg = ::strdup(ClassName(this).c_str());
        return m_msg;
    }
    const char * getDetails() const { return m_details; }
    const std::vector<String> getTrace() const { return m_trace; }

  protected:
      GeneralException() : m_msg(NULL), m_details(NULL) { }
    void setMsg(char * msg) { if (m_msg) free(m_msg); m_msg = msg; }
    void genTrace();

  private:
    mutable char * m_msg;
    char * m_details;
    std::vector<String> m_trace;

    void setDetails(const char * details) {
        if (details)
            m_details = ::strdup(details);
        else
            m_details = NULL;
    }
};

class RessourceException : public GeneralException {
  public:
      RessourceException(const String & msg, const char * details) : GeneralException(msg, details) { }
};

void ExitHelper(const String & msg, const char * fmt = NULL, ...) __attribute__((format(printf, 2, 3)));

static inline void * malloc(size_t size) {
    void * r = ::malloc(size);

    if (!r && size)
        ExitHelper("Failed to allocate memory", "%li bytes", size);

    return r;
}

static inline void * calloc(size_t count, size_t size) {
    void * r = ::calloc(count, size);

    if (!r && ((count * size) != 0))
        ExitHelper("Failed to allocate memory", "%li * %li = %li bytes", count, size, count * size);

    return r;
}

static inline void * realloc(void * previous, size_t size) {
    void * r = ::realloc(previous, size);

    if (!r && size)
        ExitHelper("Failed to re-allocate memory", "%li bytes", size);

    return r;
}

static inline void AssertHelperInner(const String & msg, const char * details = NULL) throw (GeneralException) {
    throw GeneralException(msg, details);
}

static inline void AssertHelper(const String & msg, const char * fmt, ...) __attribute__((format(printf, 2, 3)));

static inline void AssertHelper(const String & msg, const char * fmt, ...) {
    String details;
    va_list ap;
    va_start(ap, fmt);
    details.set(fmt, ap);
    va_end(ap);
    AssertHelperInner(msg, details.to_charp());
}

static inline void AssertHelper(const String & msg) {
    AssertHelperInner(msg);
}

class TestException : public GeneralException {
  public:
      TestException(const String & msg) : GeneralException(msg) { }
};

static inline void TestHelper(const String & msg) throw (TestException) {
    throw TestException(msg);
}

template<typename T>
ClassName::ClassName(T * ptr) {
    int status;
    m_demangled = abi::__cxa_demangle(typeid(*ptr).name(), 0, 0, &status);
}

};

#define Failure(msg) Balau::AssertHelper(msg);
#define FailureDetails(msg, ...) Balau::AssertHelper(msg, __VA_ARGS__);

#define IAssert(c, ...) if (!__builtin_expect(!!(c), 0)) { \
    Balau::String msg; \
    msg.set("Internal Assertion " #c " failed at %s:%i", __FILE__, __LINE__); \
    Balau::AssertHelper(msg, __VA_ARGS__); \
}

#define AAssert(c, ...) if (!__builtin_expect(!!(c), 0)) { \
    Balau::String msg; \
    msg.set("API Assertion " #c " failed at %s:%i", __FILE__, __LINE__); \
    Balau::AssertHelper(msg, __VA_ARGS__); \
}

#define RAssert(c, ...) if (!__builtin_expect(!!(c), 0)) { \
    Balau::String msg; \
    msg.set("Ressource Assertion " #c " failed at %s:%i", __FILE__, __LINE__); \
    Balau::ExitHelper(msg, __VA_ARGS__); \
}

#define EAssert(c, ...) if (!__builtin_expect(!!(c), 0)) { \
    Balau::String msg; \
    msg.set("Execution Assertion " #c " failed at %s:%i", __FILE__, __LINE__); \
    Balau::AssertHelper(msg, __VA_ARGS__); \
}

#define TAssert(c) if (!__builtin_expect(!!(c), 0)) { \
    Balau::String msg; \
    msg.set("UnitTest Assert " #c " failed at %s:%i", __FILE__, __LINE__); \
    Balau::TestHelper(msg); \
}
