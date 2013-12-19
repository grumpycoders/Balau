#include "Exceptions.h"

#define MAXTRACE 128

#ifdef _WIN32

#include <windows.h>
#include <dbghelp.h>

void Balau::GeneralException::genTrace() {
    // taken from http://stackoverflow.com/questions/5693192/win32-backtrace-from-c-code
    unsigned int   i;
    void         * stack[MAXTRACE];
    unsigned short frames;
    SYMBOL_INFO  * symbol;
    HANDLE         process;

    process = GetCurrentProcess();

    SymInitialize(process, NULL, TRUE);

    frames               = CaptureStackBackTrace(0, MAXTRACE, stack, NULL);
    symbol               = (SYMBOL_INFO *) calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
    symbol->MaxNameLen   = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    for (i = 0; i < frames; i++) {
        SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);

        int status = 0;
        String line;
#ifndef _MSC_VER
        char * demangled = abi::__cxa_demangle(symbol->Name, 0, 0, &status);
#else
		char * demangled = NULL;
#endif
		line.set("%i: 0x%08x (%s)", i, symbol->Address, status == 0 && demangled ? demangled : symbol->Name);
		if (demangled)
	        free(demangled);
        m_trace.push_back(line);
    }

    free(symbol);
}

#else

#include <execinfo.h>
#include <dlfcn.h>

void Balau::GeneralException::genTrace() {
    void * trace[MAXTRACE];
    int n = backtrace(trace, MAXTRACE);
    char ** symbols = backtrace_symbols(trace, MAXTRACE);

    if (!symbols)
        return;

    String line;
    for (int i = 0; i < n; i++)
        line += String().set("%08zx ", (uintptr_t) trace[i]);

    m_trace.push_back(line);
    Dl_info info;

    for (int i = 0; i < n; i++) {
        int status;
        String line;
        dladdr(trace[i], &info);
        long dist = ((char *) trace[i]) - ((char *) info.dli_saddr);
        char * demangled;
        if (info.dli_sname) {
            demangled = abi::__cxa_demangle(info.dli_sname, 0, 0, &status);
        } else {
            demangled = NULL;
        }
        line.set("%i: %s(%s%c0x%lx) [0x%08zx]", i, info.dli_fname, info.dli_sname ? (demangled ? (status == 0 ? demangled : info.dli_sname) : info.dli_sname) : "??", dist < 0 ? '-' : '+', dist < 0 ? -dist : dist, (uintptr_t) trace[i]);
        m_trace.push_back(line);
        if (demangled)
            free(demangled);
    }

    free(symbols);
}


#endif

static void ExitHelperInner(const Balau::String & msg, const char * details) throw (Balau::RessourceException) {
    throw Balau::RessourceException(msg, details);
}

void Balau::ExitHelper(const String & msg, const char * fmt, ...) {
    if (fmt) {
        String details;
        va_list ap;
        va_start(ap, fmt);
        details.set(fmt, ap);
        va_end(ap);
        ExitHelperInner(msg, details.to_charp());
    } else {
        ExitHelperInner(msg, NULL);
    }
}
