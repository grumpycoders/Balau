ifeq ($(SYSTEM),)
    SYSTEM = $(shell uname)
endif

TRUESYSTEM = $(shell uname)
MACHINE = $(shell uname -m)
DISTRIB = $(shell cat /etc/issue | cut -f 1 -d\  | head -1)

CC = gcc
CXX = g++
LD = g++
AS = gcc -c
AR = ar rcs

CPPFLAGS += -fPIC
LDFLAGS += -fPIC
ifeq ($(DEBUG),)
CPPFLAGS += -O3
else
CPPFLAGS += -g
LDFLAGS += -g
endif

INCLUDES = includes libcoro libeio libev

ifeq ($(SYSTEM),Darwin)
    LIBS += pthread
    CONFIG_H = darwin-config.h
    ARCH_FLAGS = -arch i386
    LIBS = -liconv
    LD = g++ -arch i386
    STRIP = strip -x
    ifeq ($(TRUESYSTEM),Linux)
        CC = i686-apple-darwin9-gcc
        CXX = i686-apple-darwin9-g++
        LD = i686-apple-darwin-g++ -arch i386 -mmacosx-version-min=10.5
        STRIP = i686-apple-darwin-strip -x
        AS = i686-apple-darwin-as -arch i386
    endif
else
ifeq ($(SYSTEM),Linux)
    LIBS += pthread
    CONFIG_H = linux-config.h
    ARCH_FLAGS = -march=i686 -m32
    ASFLAGS = -march=i686 --32
    STRIP = strip --strip-unneeded
endif
endif

CPPFLAGS_NO_ARCH += $(addprefix -I, $(INCLUDES)) -fexceptions -Wno-deprecated -imacros $(CONFIG_H)
CPPFLAGS += $(CPPFLAGS_NO_ARCH) $(ARCH_FLAGS)

LDFLAGS += $(ARCH_FLAGS) $(addprefix -l, $(LIBS))

vpath %.cc src:tests
vpath %.c libcoro:libeio:libev

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
\
Task.cc \
TaskMan.cc \

LIBCORO_SOURCES = \
coro.c \

LIBEV_SOURCES = \
ev.c \
event.c \

LIBEIO_SOURCES = \
eio.c \

TEST_SOURCES = \
test-Sanity.cc \
test-String.cc \
test-Tasks.cc \
test-Handles.cc \

LIB = libBalau.a

BALAU_OBJECTS = $(addsuffix .o, $(notdir $(basename $(BALAU_SOURCES) $(LIBCORO_SOURCES) $(LIBEIO_SOURCES) $(LIBEV_SOURCES))))

WHOLE_SOURCES = $(BALAU_SOURCES) $(LIBCORO_SOURCES) $(LIBEIO_SOURCES) $(LIBEV_SOURCES) $(TEST_SOURCES)
TESTS = $(addsuffix .bin, $(notdir $(basename $(TEST_SOURCES))))

ALL_OBJECTS = $(addsuffix .o, $(notdir $(basename $(WHOLE_SOURCES))))
ALL_DEPS = $(addsuffix .dep, $(notdir $(basename $(WHOLE_SOURCES))))

all: dep lib

tests: $(TESTS)
	for t in $(TESTS) ; do ./$$t ; done

strip: $(TESTS)
	for t in $(TESTS) ; do $(STRIP) ./$$t ; done

lib: $(LIB)

libBalau.a: $(BALAU_OBJECTS)
	$(AR) libBalau.a $(BALAU_OBJECTS)

%.bin : %.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $< ./$(LIB)

dep: $(ALL_DEPS)

%.dep : %.cc
	$(CXX) $(CPPFLAGS_NO_ARCH) -M -MF $@ $<

%.dep : %.c
	$(CC) $(CPPFLAGS_NO_ARCH) -M -MF $@ $<

-include $(ALL_DEPS)

clean:
	rm -f $(ALL_OBJECTS) $(TESTS) $(LIB) $(ALL_DEPS)

.PHONY: lib tests clean strip
