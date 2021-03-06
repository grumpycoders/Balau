#pragma once

#include <stdarg.h>
#include <BString.h>
#include <Threads.h>

namespace Balau {

enum {
    M_DEBUG = 1,
    M_INFO = 2,
    M_STATUS = 4,
    M_WARNING = 8,
    M_ERROR = 16,
    M_ALERT = 32,

    M_ALL = M_DEBUG | M_INFO | M_STATUS | M_WARNING | M_ERROR | M_ALERT,

    M_ENGINE_DEBUG = 64,
    M_MAX = M_ENGINE_DEBUG,
};

#undef E_STRING
#undef E_TASK
#undef E_EVENT
#undef E_HANDLE
#undef E_INPUT
#undef E_SOCKET
#undef E_THREAD
#undef E_SELECT

enum {
    E_STRING = 1,
    E_TASK = 2,
    E_EVENT = 4,
    E_HANDLE = 8,
    E_INPUT = 16,
    E_SOCKET = 32,
    E_THREAD = 64,
    E_OUTPUT = 128,
    E_HTTPSERVER = 256,
    E_ASYNC = 512,
    E_SELECT = 1024,
};

class Printer {
    void _print(const char * fmt, ...);
    void _log(uint32_t level, const char * fmt, va_list ap);

    Lock m_lock;
  public:
      Printer();

    void setLocal();
    void setGlobal();

    static Printer * getPrinter();
    template<typename... Args>
    static void slog(uint32_t level, const String & fmt, Args... args) { slog(level, fmt.to_charp(), args...); }
    static void log(uint32_t level, const char * fmt, ...) printfwarning(2, 3) { va_list ap; va_start(ap, fmt); vlog(level, fmt, ap); va_end(ap); }
    static void vlog(uint32_t level, const char * fmt, va_list ap) printfwarning(2, 0) { getPrinter()->_log(level, fmt, ap); }

    template<typename... Args>
	static void sprint(const String & fmt, Args... args) { sprint(fmt.to_charp(), args...); }
    static void print(const char * fmt, ...) printfwarning(1, 2) { va_list ap; va_start(ap, fmt); vprint(fmt, ap); va_end(ap); }
    static void vprint(const char * fmt, va_list ap) printfwarning(1, 0) { getPrinter()->_print(fmt, ap); }

#ifdef FULLDEBUG
    static void elog(uint32_t engine, const char * fmt, ...) printfwarning(2, 3) {
        va_list ap;
        va_start(ap, fmt);
        getPrinter()->_log(M_ENGINE_DEBUG, fmt, ap);
        va_end(ap);
    }
#else
    static void elog(uint32_t engine, const char * fmt, ...) { }
#endif

    static void enable(uint32_t levels = M_ALL) { getPrinter()->m_verbosity |= levels; }
    static void disable(uint32_t levels = M_ALL) { getPrinter()->m_verbosity &= ~levels; }

    static void setDetailled(bool enable) { getPrinter()->m_detailledLogs = enable; }

    virtual void _print(const char * fmt, va_list ap);

  private:
    uint32_t m_verbosity = M_STATUS | M_WARNING | M_ERROR | M_ENGINE_DEBUG;
    bool m_detailledLogs = false;

      Printer(const Printer &) = delete;
    Printer & operator=(const Printer &) = delete;
};

};
