#define PIC 1

#define STDC_HEADERS 1
#define WORDS_LITTLEENDIAN 1
#define CORO_SJLJ 1
#define _FILE_OFFSET_BITS 64

#define EMBED_LIBEIO 1
#define EV_STANDALONE 1

#define HAVE_DLFCN_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_MEMORY_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_SYSCALL_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_UNISTD_H 1

#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif

#define HAVE_POLL_H 1
#define HAVE_SYS_EVENT_H 1
#define HAVE_SYS_SELECT_H 1
