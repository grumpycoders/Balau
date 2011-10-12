#include <stdio.h>
#include <stdlib.h>

void __eprintf(const char * msg, const char * file, unsigned line, const char * e) {
    fprintf(stderr, msg, file, line, e);
    abort();
}
