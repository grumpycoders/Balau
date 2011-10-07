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

ifeq ($(DEBUG),)
CPPFLAGS += -O3
else
CPPFLAGS += -g
LDFLAGS += -g
endif

ifeq ($(SYSTEM),Darwin)
    LIBCORO_CFLAGS = -DCORO_SJLJ
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
    LIBCORO_CFLAGS = -DCORO_ASM
    ARCH_FLAGS = -march=i686 -m32
    ASFLAGS = -march=i686 --32
    STRIP = strip --strip-unneeded
endif

INCLUDES = -Iincludes -Ilibcoro

CPPFLAGS_NO_ARCH += $(INCLUDES) -DSTDC_HEADERS -fexceptions -DWORDS_LITTLEENDIAN $(HAVES) $(LIBCORO_CFLAGS) -Wno-deprecated -D_FILE_OFFSET_BITS=64
CPPFLAGS += $(CPPFLAGS_NO_ARCH) $(ARCH_FLAGS)

LDFLAGS += $(ARCH_FLAGS) $(LIBS)

vpath %.cc src:tests
vpath %.c libcoro

BALAU_SOURCES = \
BString.cc \
Local.cc \
Main.cc \
Printer.cc \
Task.cc \
TaskMan.cc \

LIBCORO_SOURCES = \
coro.c \

TEST_SOURCES = \
test-Sanity.cc \
test-String.cc \
test-Tasks.cc \

LIB = libBalau.a

BALAU_OBJECTS = $(addsuffix .o, $(notdir $(basename $(BALAU_SOURCES) $(LIBCORO_SOURCES))))

WHOLE_SOURCES = $(BALAU_SOURCES) $(LIBCORO_SOURCES) $(TEST_SOURCES)
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
