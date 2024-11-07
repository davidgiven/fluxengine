BUILDTYPE ?= host
export BUILDTYPE

ifeq ($(BUILDTYPE),windows)
	MINGW = i686-w64-mingw32-
	CC = $(MINGW)gcc
	CXX = $(MINGW)g++ -std=c++17
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

.PHONY: install install-bin
install:: all install-bin

clean::
	$(hide) rm -rf $(REALOBJ)

install-bin:
	@echo "INSTALL"
	$(hide) install -D -v "$(OBJ)/src+fluxengine/src+fluxengine" "$(DESTDIR)$(BINDIR)/fluxengine"
	$(hide) install -D -v "$(OBJ)/src/gui+gui/gui+gui" "$(DESTDIR)$(BINDIR)/fluxengine-gui"
	$(hide) install -D -v "$(OBJ)/tools+brother120tool/tools+brother120tool" "$(DESTDIR)$(BINDIR)/brother120tool"
	$(hide) install -D -v "$(OBJ)/tools+brother240tool/tools+brother240tool" "$(DESTDIR)$(BINDIR)/brother240tool"
	$(hide) install -D -v "$(OBJ)/tools+upgrade-flux-file/tools+upgrade-flux-file" "$(DESTDIR)$(BINDIR)/upgrade-flux-file"

include build/ab.mk
