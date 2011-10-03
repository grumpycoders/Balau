ifeq ($(SYSTEM),)
    SYSTEM = $(shell uname)
endif

TRUESYSTEM = $(shell uname)
MACHINE = $(shell uname -m)
DISTRIB = $(shell cat /etc/issue | cut -f 1 -d\  | head -1)

CC = gcc
CXX = g++
LD = g++ -m32
AS = gcc -c -m32
AR = ar rcs

ifeq ($(SYSTEM),Darwin)
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
    ARCH_FLAGS = -march=i686 -m32
    ASFLAGS = -march=i686 --32
    STRIP = strip --strip-unneeded
endif

INCLUDES = -Iincludes

CPPFLAGS_NO_ARCH += $(INCLUDES) -g -DSTDC_HEADERS -fexceptions -DWORDS_LITTLEENDIAN $(HAVES)
CPPFLAGS += $(CPPFLAGS_NO_ARCH) $(ARCH_FLAGS)

LDFLAGS += $(ARCH_FLAGS) $(LIBS)

vpath %.cc src:tests

BALAU_SOURCES = \
BString.cc \
Local.cc \
Main.cc \
Printer.cc \

TEST_SOURCES = \
test-String.cc \

LIB = libBalau.a

WHOLE_SOURCES = $(BALAU_SOURCES) $(TEST_SOURCES)
TESTS = $(notdir $(basename $(TEST_SOURCES)))

BALAU_OBJECTS = $(addsuffix .o, $(notdir $(basename $(BALAU_SOURCES))))
ALL_OBJECTS = $(addsuffix .o, $(notdir $(basename $(WHOLE_SOURCES))))
ALL_DEPS = $(addsuffix .dep, $(notdir $(basename $(WHOLE_SOURCES))))

all: dep lib

tests: $(TESTS)
	for t in $(TESTS) ; do ./$$t ; done

lib: $(LIB)

libBalau.a: $(BALAU_OBJECTS)
	$(AR) libBalau.a $(BALAU_OBJECTS)

test-String: test-String.o $(LIB)
	$(LD) -o $@ $< ./$(LIB)

dep: $(ALL_DEPS)

%.dep : %.cc
	$(CXX) $(CPPFLAGS_NO_ARCH) -M -MF $@ $<

-include $(ALL_DEPS)

clean:
	rm -f $(ALL_OBJECTS) $(TESTS) $(LIB) $(ALL_DEPS)

.PHONY: lib tests clean
