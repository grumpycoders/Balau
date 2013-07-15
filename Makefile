ifeq ($(SYSTEM),)
    SYSTEM := $(shell uname | cut -f 1 -d_)
endif

TRUESYSTEM := $(shell uname)
MACHINE := $(shell uname -m)
DISTRIB := $(shell cat /etc/issue | cut -f 1 -d\  | head -1)

CC = gcc
CXX = g++
LD = g++
AS = gcc -c
AR = ar rcs

BINEXT = bin

CPPFLAGS += -fno-strict-aliasing

ifeq ($(DEBUG),)
CPPFLAGS += -g -O3 -DNDEBUG
LDFLAGS += -g
else
CPPFLAGS += -g -DDEBUG -DEV_VERIFY=3
LDFLAGS += -g
endif

INCLUDES = includes libcoro libev LuaJIT/src
LIBS = z
DEFINES = _LARGEFILE64_SOURCE

ifeq ($(SYSTEM),Darwin)
    CC = clang
    CXX = clang++
    CPPFLAGS += -fPIC
    LDFLAGS += -fPIC
    LIBS += pthread iconv
    CONFIG_H = darwin-config.h
    ARCH_FLAGS =
    LD = clang++
    STRIP = strip -x
endif

ifeq ($(SYSTEM),Linux)
    CPPFLAGS += -fPIC
    LDFLAGS += -fPIC -rdynamic
    LIBS += pthread dl
    CONFIG_H = linux-config.h
    ARCH_FLAGS =
    ASFLAGS =
    STRIP = strip --strip-unneeded

    GCC_VERSION := $(shell g++ -dumpversion)
    GCC_VERSION_4 := $(shell expr `g++ -dumpversion | cut -f1 -d.` \>= 4)
    GCC_VERSION_x_8 := $(shell expr `g++ -dumpversion | cut -f2 -d.` >= 8)

    ifneq ($(GCC_VERSION_4),1)
        USE_CLANG = true
    else
        ifneq ($(GCC_VERSION_x_8),1)
            USE_CLANG = true
        endif
    endif

    ifeq ($(USE_CLANG),true)
        CC = clang
        CXX = clang++
        LD = clang++
    endif
endif

CPPFLAGS_NO_ARCH += $(addprefix -I, $(INCLUDES)) -fexceptions -imacros $(CONFIG_H)
CPPFLAGS += $(CPPFLAGS_NO_ARCH) $(ARCH_FLAGS) $(addprefix -D, $(DEFINES))

CXXFLAGS += -Wno-deprecated -std=c++11

LDFLAGS += $(ARCH_FLAGS)
LDLIBS = $(addprefix -l, $(LIBS))

vpath %.cc src:tests
vpath %.c libcoro:libev:win32/pthreads-win32:win32/iconv:win32/regex

BALAU_SOURCES = \
Exceptions.cc \
\
Local.cc \
Threads.cc \
Async.cc \
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
ZHandle.cc \
\
Task.cc \
TaskMan.cc \
\
Http.cc \
HttpServer.cc \
SimpleMustache.cc \
\
BLua.cc \
\
LuaTask.cc \
\
BRegex.cc \

ifeq ($(SYSTEM),MINGW32)
WIN32_SOURCES = \
pthread.c \
iconv.c \
localcharset.c \
relocatable.c \
msvc-regex.c \
win32-vsscanf.c \

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

TEST_SOURCES = \
test-Sanity.cc \
test-String.cc \
test-Tasks.cc \
test-Threads.cc \
test-Async.cc \
test-Handles.cc \
test-Sockets.cc \
test-Http.cc \
test-Lua.cc \
test-Regex.cc \

LIB = libBalau.a

BALAU_OBJECTS = $(addsuffix .o, $(notdir $(basename $(BALAU_SOURCES) $(LIBCORO_SOURCES) $(LIBEV_SOURCES) $(WIN32_SOURCES) $(DARWIN_SOURCES))))

WHOLE_SOURCES = $(BALAU_SOURCES) $(LIBCORO_SOURCES) $(LIBEV_SOURCES) $(WIN32_SOURCES) $(DARWIN_SOURCES) $(TEST_SOURCES)
TESTS = $(addsuffix .$(BINEXT), $(notdir $(basename $(TEST_SOURCES))))

ALL_OBJECTS = $(addsuffix .o, $(notdir $(basename $(WHOLE_SOURCES))))
ALL_DEPS = $(addsuffix .dep, $(notdir $(basename $(WHOLE_SOURCES))))

all: dep lib

tests: $(TESTS)
ifneq ($(CROSSCOMPILE),true)
	$(foreach b, $(TESTS), ./$(b) && ) exit 0 || exit 1
else
ifeq ($(SYSTEM),MINGW32)
	$(foreach b, $(TESTS), wine ./$(b) && ) exit 0 || exit 1
endif
endif

strip: $(TESTS)
	$(foreach b, $(TESTS), $(STRIP) ./$(b) && ) exit 0 || exit 1

lib: $(LIB)

LuaJIT:
ifeq ($(CROSSCOMPILE),true)
	$(MAKE) -C LuaJIT HOST_CC="gcc -m32" CROSS=$(LUAJIT_CROSS) TARGET_SYS=$(LUAJIT_TARGET) BUILDMODE=static
else
	$(MAKE) -C LuaJIT CC="$(CC) $(ARCH_FLAGS)" BUILDMODE=static
endif

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
