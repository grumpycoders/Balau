#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>
#include <string.h>

int vsscanf(const char * buffer, const char * format, va_list args) {
    size_t count = 0;
    const char * p = format;
    char c;
    while ((c = *p++))
        if ((c == '%') && ((p[0]) != '*' && (p[0] != '%')))
            count++;

    const char ** new_stack = (const char **) alloca((count + 2) * sizeof(void *));

    new_stack[0] = buffer;
    new_stack[1] = format;
    memcpy(new_stack + 2, args, count * sizeof(const char *));

    int r;
    void * old_stack;

#ifdef __x86_64__
#error "Not written yet."
#else
    asm (
        "mov  %%esp, %0\n\t"
        "mov  %2, %%esp\n\t"
        "call sscanf\n\t"
        "mov  %0, %%esp\n\t"
        "mov  %%eax, %1\n\t"
        : "=r"(old_stack), "=r"(r)
        : "r"(new_stack)
    );
#endif

    return r;
}
