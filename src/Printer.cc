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
};

Balau::Printer::Printer() : m_verbosity(M_STATUS | M_WARNING | M_ERROR) {
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

    Printer * printer = getPrinter();

    printer->_print(prefixes[i]);
    printer->_print(fmt, ap);
    printer->_print("\n");
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
