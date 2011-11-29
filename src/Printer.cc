#include "ev++.h"
#include "Printer.h"
#include "Main.h"
#include "Local.h"

static Balau::DefaultTmpl<Balau::Printer> defaultPrinter(10);
static Balau::LocalTmpl<Balau::Printer> localPrinter;

static const char * prefixes[] = {
    "(DD) ",
    "(II) ",
    "(--) ",
    "(WW) ",
    "(EE) ",
    "(AA) ",
    "(**) ",
};

Balau::Printer::Printer() : m_verbosity(M_STATUS | M_WARNING | M_ERROR | M_ENGINE_DEBUG), m_detailledLogs(false) {
#ifdef DEBUG
    m_detailledLogs = true;
#endif
    if (!localPrinter.getGlobal())
        localPrinter.setGlobal(this);
}

void Balau::Printer::setLocal() {
    localPrinter.set(this);
}

Balau::Printer * Balau::Printer::getPrinter() { return localPrinter.get(); }

void Balau::Printer::_log(uint32_t level, const char * fmt, va_list ap) {
    if (!(level & m_verbosity))
        return;

    int l, i;

    for (l = M_MAX, i = (sizeof(prefixes) / sizeof(*prefixes)) - 1; l; l >>= 1, i--)
        if (l & level)
            break;

    m_lock.enter();
    if (m_detailledLogs) {
        struct ev_loop * loop = ev_default_loop(0);
        ev_now_update(loop);
        ev_tstamp now = ev_now(loop);
        _print("[%15.8f:%08x] ", now, pthread_self());
    }
    _print(prefixes[i]);
    _print(fmt, ap);
    _print("\n");
    m_lock.leave();
}

void Balau::Printer::_print(const char * fmt, va_list ap) {
    vfprintf(stderr, fmt, ap);
}

void Balau::Printer::_print(const char * fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    _print(fmt, ap);
    va_end(ap);
}
