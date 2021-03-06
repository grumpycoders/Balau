include common.mk

ifeq ($(DEBUG),)
CPPFLAGS += -g3 -gdwarf-2 -O3 -DNDEBUG
LDFLAGS += -g3 -gdwarf-2
else
CPPFLAGS += -g3 -gdwarf-2 -D_DEBUG -DEV_VERIFY=3
LDFLAGS += -g3 -gdwarf-2
endif

INCLUDES = includes libev LuaJIT/src lcrypt libtommath libtomcrypt/src/headers src/jsoncpp/include
LIBS = z curl cares
DEFINES = _LARGEFILE64_SOURCE LITTLE_ENDIAN LTM_DESC LTC_SOURCE USE_LTM LTC_NO_ROLC

ifeq ($(SYSTEM),Darwin)
    LIBS += pthread iconv
    CONFIG_H = darwin-config.h
endif

ifeq ($(SYSTEM),Linux)
    LIBS += pthread dl
    CONFIG_H = linux-config.h
endif

CPPFLAGS_NO_ARCH += $(addprefix -I, $(INCLUDES)) -fexceptions -imacros $(ROOT_DIR)/$(CONFIG_H)
CPPFLAGS += $(CPPFLAGS_NO_ARCH) $(ARCH_FLAGS) $(addprefix -D, $(DEFINES))

LDFLAGS += $(ARCH_FLAGS)
LDLIBS = $(addprefix -l, $(LIBS))

vpath %.cpp src/jsoncpp/src
vpath %.cc src:tests
vpath %.c libev:win32/pthreads-win32:win32/iconv:win32/regex:lcrypt

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
BigInt.cc \
BRegex.cc \
\
Handle.cc \
Input.cc \
Output.cc \
MMap.cc \
Socket.cc \
Selectable.cc \
SmartWriter.cc \
Buffer.cc \
BStream.cc \
ZHandle.cc \
\
StdIO.cc \
\
Task.cc \
TaskMan.cc \
\
HelperTasks.cc \
\
CurlTask.cc \
\
Http.cc \
HttpServer.cc \
HttpActionStatic.cc \
SimpleMustache.cc \
BWebSocket.cc \
\
json_reader.cpp \
json_writer.cpp \
json_value.cpp \
\
Base64.cc \
\
BLua.cc \
LuaBigInt.cc \
LuaHandle.cc \
LuaTask.cc \
\
lcrypt.c \

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

LIBEV_SOURCES = \
ev.c \
event.c \

TEST_SOURCES = \
test-Sanity.cc \
test-BigInt.cc \
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

BALAU_OBJECTS = $(addsuffix .o, $(notdir $(basename $(BALAU_SOURCES) $(LIBEV_SOURCES) $(WIN32_SOURCES) $(DARWIN_SOURCES))))

WHOLE_SOURCES = $(BALAU_SOURCES) $(LIBEV_SOURCES) $(WIN32_SOURCES) $(DARWIN_SOURCES) $(TEST_SOURCES)
TESTS = $(addsuffix .$(BINEXT), $(notdir $(basename $(TEST_SOURCES))))

ALL_OBJECTS = $(addsuffix .o, $(notdir $(basename $(WHOLE_SOURCES))))
ALL_DEPS = $(addsuffix .dep, $(notdir $(basename $(WHOLE_SOURCES))))

all: dep lib

build_tests: $(TESTS)

test: build_tests
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

LuaJIT/src/libluajit.a:
ifeq ($(CROSSCOMPILE),true)
	$(MAKE) -C LuaJIT HOST_CC="gcc -m32" CROSS=$(LUAJIT_CROSS) TARGET_SYS=$(LUAJIT_TARGET) BUILDMODE=static
else
	$(MAKE) -C LuaJIT CC="$(CC) $(ARCH_FLAGS)" BUILDMODE=static
endif

libtommath: libtommath/libtommath.a

libtommath/libtommath.a:
	$(MAKE) -C libtommath CC="$(CC) $(ARCH_FLAGS)"

libtomcrypt: libtomcrypt/libtomcrypt.a

libtomcrypt/libtomcrypt.a:
	$(MAKE) -C libtomcrypt CC="$(CC) $(ARCH_FLAGS) -DLTM_DESC -DUSE_LTM -DLTC_NO_ROLC -I../libtommath"

LuaJIT: LuaJIT/src/libluajit.a

libBalau.a: LuaJIT/src/libluajit.a libtommath/libtommath.a libtomcrypt/libtomcrypt.a $(BALAU_OBJECTS)
ifeq ($(SYSTEM),Darwin)
ifneq ($(CROSSCOMPILE),true)
	rm -f libBalau.a
endif
endif
	$(AR) libBalau.a $(BALAU_OBJECTS)

%.$(BINEXT) : %.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $< ./$(LIB) ./LuaJIT/src/libluajit.a ./libtomcrypt/libtomcrypt.a ./libtommath/libtommath.a $(LDLIBS)

dep: $(ALL_DEPS)

%.dep : %.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS_NO_ARCH) -M $< > $@

%.dep : %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS_NO_ARCH) -M $< > $@

%.dep : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS_NO_ARCH) -M $< > $@

-include $(ALL_DEPS)

clean:
	rm -f $(ALL_OBJECTS) $(TESTS) $(LIB) $(ALL_DEPS)
	$(MAKE) -C LuaJIT clean
	$(MAKE) -C libtommath clean
	$(MAKE) -C libtomcrypt clean

.PHONY: lib test build_tests clean strip LuaJIT libtommath libtomcrypt
