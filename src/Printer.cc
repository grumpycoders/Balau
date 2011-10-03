#include "Printer.h"
#include "Main.h"
#include "Local.h"

class PrinterLocal : public Balau::Local {
  public:
      PrinterLocal() { }
    Balau::Printer * getGlobal() { return reinterpret_cast<Balau::Printer *>(Local::getGlobal()); }
    Balau::Printer * get() { return reinterpret_cast<Balau::Printer *>(Local::get()); }
    void setGlobal(Balau::Printer * printer) { Local::setGlobal(printer); }
    void set(Balau::Printer * printer) { Local::set(printer); }
} printerLocal;

static const char * prefixes[] = {
    "(DD) ",
    "(II) ",
    "(--) ",
    "(WW) ",
    "(EE) ",
    "(AA) ",
};

Balau::Printer::Printer() : m_verbosity(M_STATUS | M_WARNING | M_ERROR) {
    if (!printerLocal.getGlobal())
        printerLocal.setGlobal(this);
}

Balau::Printer * Balau::Printer::getPrinter() { return printerLocal.get(); }

void Balau::Printer::_log(uint32_t level, const char * fmt, va_list ap) {
    if (!(level & m_verbosity))
        return;

    int l, i;

    for (l = M_MAX, i = (sizeof(prefixes) / sizeof(*prefixes)) - 1; l; l >>= 1, i--)
        if (l & level)
            break;

    print(prefixes[i]);
    print(fmt, ap);
    print("\n");
}

void Balau::Printer::_print(const char * fmt, va_list ap) {
    vfprintf(stderr, fmt, ap);
}

class DefaultPrinter : public Balau::AtStart {
  public:
      DefaultPrinter() : AtStart(10) { }
  protected:
    virtual void doStart();
};

static DefaultPrinter defaultPrinter;

void DefaultPrinter::doStart() {
    new Balau::Printer();
}
