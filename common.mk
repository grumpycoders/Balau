ROOT_DIR := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

ifeq ($(SYSTEM),)
    SYSTEM := $(shell uname | cut -f 1 -d_)
endif

TRUESYSTEM := $(shell uname)
MACHINE := $(shell uname -m)
DISTRIB := $(shell cat /etc/issue | cut -f 1 -d\  | head -1)

CC = clang
CXX = clang++
LD = clang++
AS = clang -c
AR = ar rcs

BINEXT = bin

CPPFLAGS += -fno-strict-aliasing

ifeq ($(SYSTEM),Darwin)
    CPPFLAGS += -fPIC
    LDFLAGS += -fPIC
    ARCH_FLAGS =
    STRIP = strip -x
endif

ifeq ($(SYSTEM),Linux)
    CPPFLAGS += -fPIC
    LDFLAGS += -fPIC -rdynamic
    ARCH_FLAGS =
    ASFLAGS =
    STRIP = strip --strip-unneeded
endif

CXXFLAGS += -Wno-deprecated -std=c++11
