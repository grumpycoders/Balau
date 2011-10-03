#pragma once

#include <BString.h>

namespace Balau {

enum {
    M_DEBUG = 1,
    M_INFO = 2,
    M_STATUS = 4,
    M_WARNING = 8,
    M_ERROR = 16,
    M_ALERT = 32,

    M_ALL = M_DEBUG | M_INFO | M_STATUS | M_WARNING | M_ERROR | M_ALERT,
    M_MAX = M_ALERT,
};

class Printer {
  protected:
    virtual void _print(const char * fmt, va_list ap);

  private:
    void _log(uint32_t level, const char * fmt, va_list ap);
    static Printer * getPrinter();

  public:
      Printer();

    void setLocal();

    static void log(uint32_t level, const String & fmt, ...) { va_list ap; va_start(ap, fmt); log(level, fmt.to_charp(), ap); va_end(ap); }
    static void log(uint32_t level, const char * fmt, ...) { va_list ap; va_start(ap, fmt); log(level, fmt, ap); va_end(ap); }
    static void log(uint32_t level, const char * fmt, va_list ap) { getPrinter()->_log(level, fmt, ap); }
    static void print(const String & fmt, ...) { va_list ap; va_start(ap, fmt); print(fmt.to_charp(), ap); va_end(ap); }
    static void print(const char * fmt, ...) { va_list ap; va_start(ap, fmt); print(fmt, ap); va_end(ap); }
    static void print(const char * fmt, va_list ap) { getPrinter()->_print(fmt, ap); }

    void enable(uint32_t levels = M_ALL) { m_verbosity |= levels; }
    void disable(uint32_t levels = M_ALL) { m_verbosity &= ~levels; }

    uint32_t m_verbosity;
};

};
