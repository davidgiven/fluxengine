ifeq ($(BUILDTYPE),)
    buildtype_Darwin = osx
    buildtype_Haiku = haiku
    BUILDTYPE := $(buildtype_$(shell uname -s ))
        ifeq ($(BUILDTYPE),)
                BUILDTYPE := unix
        endif
endif
export BUILDTYPE

ifeq ($(BUILDTYPE),windows)
	MINGW = i686-w64-mingw32-
	CC = $(MINGW)gcc
	CXX = $(MINGW)g++ -std=c++20
	CFLAGS += -g -O3
	CXXFLAGS += \
		-fext-numeric-literals \
		-Wno-deprecated-enum-float-conversion \
		-Wno-deprecated-enum-enum-conversion
	LDFLAGS += -static
	AR = $(MINGW)ar
	PKG_CONFIG = $(MINGW)pkg-config -static
	WINDRES = $(MINGW)windres
	WX_CONFIG = /usr/i686-w64-mingw32/sys-root/mingw/bin/wx-config-3.0 --static=yes
	EXT = .exe
else
	CC = gcc
	CXX = g++ -std=c++17
	CFLAGS = -g -O3
	LDFLAGS =
	AR = ar
	PKG_CONFIG = pkg-config
	ifeq ($(BUILDTYPE),osx)
	else
		LDFLAGS += -pthread -Wl,--no-as-needed
	endif

endif

HOSTCC = gcc
HOSTCXX = g++ -std=c++17
HOSTCFLAGS = -g -O3
HOSTLDFLAGS =

REALOBJ = .obj
OBJ = $(REALOBJ)/$(BUILDTYPE)
DESTDIR ?=
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

# Special Windows settings.

ifeq ($(OS), Windows_NT)
	EXT ?= .exe
	MINGWBIN = /mingw32/bin
	CCPREFIX = $(MINGWBIN)/
	PKG_CONFIG = $(MINGWBIN)/pkg-config
	WX_CONFIG = /usr/bin/sh $(MINGWBIN)/wx-config --static=yes
	PROTOC = $(MINGWBIN)/protoc
	WINDRES = windres
	LDFLAGS += \
		-static
	CXXFLAGS += \
		-fext-numeric-literals \
		-Wno-deprecated-enum-float-conversion \
		-Wno-deprecated-enum-enum-conversion

	# Required to get the gcc run - time libraries on the path.
	export PATH := $(PATH):$(MINGWBIN)
endif

# Special OSX settings.

ifeq ($(shell uname),Darwin)
	LDFLAGS += \
		-framework IOKit \
		-framework Foundation 
endif

.PHONY: all
all: +all README.md

.PHONY: binaries tests
binaries: all
tests: all
	
README.md: $(OBJ)/scripts/+mkdocindex/mkdocindex$(EXT)
	@echo $(PROGRESSINFO) MKDOC $@
	@csplit -s -f$(OBJ)/README. README.md '/<!-- FORMATSSTART -->/' '%<!-- FORMATSEND -->%'
	@(cat $(OBJ)/README.00 && $< && cat $(OBJ)/README.01) > README.md

.PHONY: tests

clean::
	$(hide) rm -rf $(REALOBJ)

include build/ab.mk

DOCKERFILES = \
	debian11 \
    debian12 \
    fedora40 \
    fedora41 \
	manjaro

docker-%: tests/docker/Dockerfile.%
	docker build -t $* -f $< .

.PHONY: dockertests
dockertests: $(foreach f,$(DOCKERFILES), docker-$(strip $f) .WAIT)
