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

ifeq ($(SYSTEM),Darwin)
    CC = clang
    CXX = clang++
    CPPFLAGS += -fPIC
    LDFLAGS += -fPIC
    ARCH_FLAGS =
    LD = clang++
    STRIP = strip -x
endif

ifeq ($(SYSTEM),Linux)
    CPPFLAGS += -fPIC
    LDFLAGS += -fPIC -rdynamic
    ARCH_FLAGS =
    ASFLAGS =
    STRIP = strip --strip-unneeded

    GCC_VERSION := $(shell g++ -dumpversion)
    GCC_VERSION_4 := $(shell expr `g++ -dumpversion | cut -f1 -d.` \>= 4)
    GCC_VERSION_x_8 := $(shell expr `g++ -dumpversion | cut -f2 -d.` \>= 8)

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

CXXFLAGS += -Wno-deprecated -std=c++11
