ifeq ($(SYSTEM),)
    SYSTEM = $(shell uname | cut -f 1 -d_)
endif

TRUESYSTEM = $(shell uname)
MACHINE = $(shell uname -m)
DISTRIB = $(shell cat /etc/issue | cut -f 1 -d\  | head -1)

CC = gcc
CXX = g++
LD = g++
AS = gcc -c
AR = ar rcs

BINEXT = bin

CPPFLAGS += -fno-strict-aliasing

ifeq ($(DEBUG),)
CPPFLAGS += -O3 -DNDEBUG
else
CPPFLAGS += -g -DDEBUG
LDFLAGS += -g
endif

INCLUDES = includes libcoro libeio libev LuaJIT/src
LIBS =

ifeq ($(SYSTEM),Darwin)
    CC = clang
    CXX = clang++
    CPPFLAGS += -fPIC
    LDFLAGS += -fPIC
    LIBS += pthread iconv
    CONFIG_H = darwin-config.h
    ARCH_FLAGS = -arch i386
    LD = clang++ -arch i386
    STRIP = strip -x
    ifeq ($(TRUESYSTEM),Linux)
        CROSSCOMPILE = true
        ARCH_FLAGS =
        CC = i686-apple-darwin9-gcc
        CXX = i686-apple-darwin9-g++
        LD = i686-apple-darwin9-g++ -arch i386 -mmacosx-version-min=10.5
        STRIP = i686-apple-darwin9-strip -x
        AS = i686-apple-darwin9-as -arch i386
        AR = i686-apple-darwin9-ar rcs
    endif
endif

ifeq ($(SYSTEM),MINGW32)
    BINEXT = exe
    COMPILE_PTHREADS = true
    CONFIG_H = mingw32-config.h
    INCLUDES += win32/iconv win32/pthreads-win32
    LIBS += ws2_32
    ifeq ($(TRUESYSTEM),Linux)
        ifeq ($(DISTRIB),CentOS)
            CROSSCOMPILE = true
            CC = i686-pc-mingw32-gcc
            CXX = i686-pc-mingw32-g++
            LD = i686-pc-mingw32-g++
            AS = i686-pc-mingw32-gcc -c
            STRIP = i686-pc-mingw32-strip --strip-unneeded
            WINDRES = i686-pc-mingw32-windres
            AR = i686-pc-mingw32-ar rcs
        else
            CROSSCOMPILE = true
            CC = i586-mingw32msvc-gcc
            CXX = i586-mingw32msvc-g++
            LD = i586-mingw32msvc-g++
            AS = i586-mingw32msvc-gcc -c
            STRIP = i586-mingw32msvc-strip --strip-unneeded
            WINDRES = i586-mingw32msvc-windres
            AR = i586-mingw32msvc-ar rcs
        endif
    endif

    ifeq ($(TRUESYSTEM),Darwin)
        CROSSCOMPILE = true
        CC = i386-mingw32-gcc
        CXX = i386-mingw32-g++
        LD = i386-mingw32-g++
        AS = i386-mingw32-gcc -c
        STRIP = i386-mingw32-strip --strip-unneeded
        WINDRES = i386-mingw32-windres
        AR = i386-mingw32-ar rcs
    endif
endif

ifeq ($(SYSTEM),Linux)
    CPPFLAGS += -fPIC
    LDFLAGS += -fPIC
    LIBS += pthread dl
    CONFIG_H = linux-config.h
    ARCH_FLAGS = -march=i686 -m32
    ASFLAGS = -march=i686 --32
    STRIP = strip --strip-unneeded
endif

CPPFLAGS_NO_ARCH += $(addprefix -I, $(INCLUDES)) -fexceptions -imacros $(CONFIG_H)
CPPFLAGS += $(CPPFLAGS_NO_ARCH) $(ARCH_FLAGS)

CXXFLAGS += -Wno-deprecated

LDFLAGS += $(ARCH_FLAGS)
LDLIBS = $(addprefix -l, $(LIBS))

vpath %.cc src:tests
vpath %.c libcoro:libeio:libev:win32/pthreads-win32:win32/iconv

BALAU_SOURCES = \
Local.cc \
Threads.cc \
\
BString.cc \
Main.cc \
Printer.cc \
\
Handle.cc \
Input.cc \
Output.cc \
Socket.cc \
Buffer.cc \
BStream.cc \
\
Task.cc \
TaskMan.cc \
\
HttpServer.cc \
\
BLua.cc \

ifeq ($(SYSTEM),MINGW32)
WIN32_SOURCES = \
pthread.c \
iconv.c \
localcharset.c \
relocatable.c \

endif

ifeq ($(SYSTEM),Darwin)
DARWIN_SOURCES = \
darwin-eprintf.c \

endif

ifneq ($(SYSTEM),MINGW32)
LIBCORO_SOURCES = \
coro.c \

endif

LIBEV_SOURCES = \
ev.c \
event.c \

LIBEIO_SOURCES = \
eio.c \

TEST_SOURCES = \
test-Sanity.cc \
test-String.cc \
test-Tasks.cc \
test-Threads.cc \
test-Handles.cc \
test-Sockets.cc \
test-Lua.cc \

LIB = libBalau.a

BALAU_OBJECTS = $(addsuffix .o, $(notdir $(basename $(BALAU_SOURCES) $(LIBCORO_SOURCES) $(LIBEIO_SOURCES) $(LIBEV_SOURCES) $(WIN32_SOURCES) $(DARWIN_SOURCES))))

WHOLE_SOURCES = $(BALAU_SOURCES) $(LIBCORO_SOURCES) $(LIBEIO_SOURCES) $(LIBEV_SOURCES) $(WIN32_SOURCES) $(DARWIN_SOURCES) $(TEST_SOURCES)
TESTS = $(addsuffix .$(BINEXT), $(notdir $(basename $(TEST_SOURCES))))

ALL_OBJECTS = $(addsuffix .o, $(notdir $(basename $(WHOLE_SOURCES))))
ALL_DEPS = $(addsuffix .dep, $(notdir $(basename $(WHOLE_SOURCES))))

all: dep lib

tests: $(TESTS)
ifneq ($(CROSSCOMPILE),true)
	for t in $(TESTS) ; do ./$$t ; done
else
ifeq ($(SYSTEM),MINGW32)
	for t in $(TESTS) ; do wine ./$$t ; done
endif
endif

strip: $(TESTS)
	for t in $(TESTS) ; do $(STRIP) ./$$t ; done

lib: $(LIB)

LuaJIT:
	$(MAKE) -C LuaJIT CC="$(CC) $(ARCH_FLAGS)" BUILDMODE=static

libBalau.a: LuaJIT $(BALAU_OBJECTS)
ifeq ($(SYSTEM),Darwin)
ifneq ($(CROSSCOMPILE),true)
	rm -f libBalau.a
endif
endif
	$(AR) libBalau.a $(BALAU_OBJECTS)

%.$(BINEXT) : %.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $< ./$(LIB) ./LuaJIT/src/libluajit.a $(LDLIBS)

dep: $(ALL_DEPS)

%.dep : %.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS_NO_ARCH) -M $< > $@

%.dep : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS_NO_ARCH) -M $< > $@

-include $(ALL_DEPS)

clean:
	rm -f $(ALL_OBJECTS) $(TESTS) $(LIB) $(ALL_DEPS)
	$(MAKE) -C LuaJIT clean

.PHONY: lib tests clean strip LuaJIT
