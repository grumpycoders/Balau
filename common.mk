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

    GCC_VERSION := $(shell g++ --version | head -1 | sed "s/.*\([0-9]\.[0-9]\.[0-9]\).*/\1/")
    GCC_VERSION_4 := $(shell expr `echo $(GCC_VERSION) | cut -f1 -d.` \>= 4)
    GCC_VERSION_x_7 := $(shell expr `echo $(GCC_VERSION) | cut -f2 -d.` \>= 7)
    GCC_VERSION_x_8 := $(shell expr `echo $(GCC_VERSION) | cut -f2 -d.` \>= 8)
    GCC_VERSION_x_x_2 := $(shell expr `echo $(GCC_VERSION) | cut -f3 -d.` \>= 2)
    CLANG_VERSION := $(shell clang --version | head -1 | sed "s/.*\([0-9]\.[0-9]\).*/\1/")
    CLANG_VERSION_3 := $(shell expr `echo $(CLANG_VERSION) | cut -f1 -d.` \>= 3)
    CLANG_VERSION_x_1 := $(shell expr `echo $(CLANG_VERSION) | cut -f2 -d.` \>= 1)

    ifneq ($(GCC_VERSION_4),1)
        USE_CLANG = true
    else
        ifneq ($(GCC_VERSION_x_7),1)
            USE_CLANG = true
        else
            ifneq ($(GCC_VERSION_x_8),1)
                ifneq ($(GCC_VERSION_x_x_2),1)
                    USE_CLANG = true
                endif
            endif
        endif
    endif

    ifeq ($(USE_CLANG),true)
        CC = clang
        CXX = clang++
        LD = clang++
    endif
endif

CXXFLAGS += -Wno-deprecated -std=c++11
