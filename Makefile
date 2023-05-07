# Special Windows settings.

ifeq ($(OS), Windows_NT)
	MINGWBIN = /mingw32/bin
	CCPREFIX = $(MINGWBIN)/
	LUA = $(MINGWBIN)/lua
	PKG_CONFIG = $(MINGWBIN)/pkg-config
	WX_CONFIG = /usr/bin/sh $(MINGWBIN)/wx-config --static=yes
	PROTOC = $(MINGWBIN)/protoc
	PLATFORM = WINDOWS
	LDFLAGS += \
		-static
	CXXFLAGS += \
		-std=c++17 \
		-fext-numeric-literals \
		-Wno-deprecated-enum-float-conversion \
		-Wno-deprecated-enum-enum-conversion

	# Required to get the gcc run-time libraries on the path.
	export PATH := $(PATH):$(MINGWBIN)
	EXT ?= .exe
endif

# Special OSX settings.

ifeq ($(shell uname),Darwin)
	PLATFORM = OSX
	LDFLAGS += \
		-framework IOKit \
		-framework Foundation
endif

# Check the Make version.


ifeq ($(findstring 4.,$(MAKE_VERSION)),)
$(error You need GNU Make 4.x for this (if you're on OSX, use gmake).)
endif

# Normal settings.

OBJDIR ?= .obj
CCPREFIX ?=
LUA ?= lua
CC ?= $(CCPREFIX)gcc
CXX ?= $(CCPREFIX)g++
AR ?= $(CCPREFIX)ar
PKG_CONFIG ?= pkg-config
WX_CONFIG ?= wx-config
PROTOC ?= protoc
CFLAGS ?= -g -O0
CXXFLAGS += -std=c++17
LDFLAGS ?=
PLATFORM ?= UNIX
TESTS ?= yes
EXT ?=
DESTDIR ?=
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

CFLAGS += \
	-Iarch \
	-Ilib \
	-I. \
	-I$(OBJDIR)/arch \
	-I$(OBJDIR)/lib \
	-I$(OBJDIR) \

LDFLAGS += \
	-lz \
	-lfmt

.SUFFIXES:
.DELETE_ON_ERROR:

define nl

endef

use-library = $(eval $(use-library-impl))
define use-library-impl
$1: $(call $3_LIB)
$1: private LDFLAGS += $(call $3_LDFLAGS)
$2: private CFLAGS += $(call $3_CFLAGS)
endef

use-pkgconfig = $(eval $(use-pkgconfig-impl))
define use-pkgconfig-impl
ifneq ($(strip $(shell $(PKG_CONFIG) $3; echo $$?)),0)
$$(error Missing required pkg-config dependency: $3)
endif

$(1): private LDFLAGS += $(shell $(PKG_CONFIG) --libs $(3))
$(2): private CFLAGS += $(shell $(PKG_CONFIG) --cflags $(3))
endef

.PHONY: all binaries tests clean install install-bin
all: binaries tests

PROTOS = \
	arch/aeslanier/aeslanier.proto \
	arch/agat/agat.proto \
	arch/amiga/amiga.proto \
	arch/apple2/apple2.proto \
	arch/brother/brother.proto \
	arch/c64/c64.proto \
	arch/f85/f85.proto \
	arch/fb100/fb100.proto \
	arch/ibm/ibm.proto \
	arch/macintosh/macintosh.proto \
	arch/micropolis/micropolis.proto \
	arch/mx/mx.proto \
	arch/northstar/northstar.proto \
	arch/rolandd20/rolandd20.proto \
	arch/smaky6/smaky6.proto \
	arch/tids990/tids990.proto \
	arch/victor9k/victor9k.proto \
	arch/zilogmcz/zilogmcz.proto \
	lib/common.proto \
	lib/config.proto \
	lib/decoders/decoders.proto \
	lib/drive.proto \
	lib/encoders/encoders.proto \
	lib/fl2.proto \
	lib/fluxsink/fluxsink.proto \
	lib/fluxsource/fluxsource.proto \
	lib/imagereader/imagereader.proto \
	lib/imagewriter/imagewriter.proto \
	lib/layout.proto \
	lib/usb/usb.proto \
	lib/vfs/vfs.proto \
	tests/testproto.proto \

PROTO_HDRS = $(patsubst %.proto, $(OBJDIR)/%.pb.h, $(PROTOS))
PROTO_SRCS = $(patsubst %.proto, $(OBJDIR)/%.pb.cc, $(PROTOS))
PROTO_OBJS = $(patsubst %.cc, %.o, $(PROTO_SRCS))
PROTO_CFLAGS = $(shell $(PKG_CONFIG) --cflags protobuf)
$(PROTO_SRCS): | $(PROTO_HDRS)
$(PROTO_OBJS): CFLAGS += $(PROTO_CFLAGS)
PROTO_LIB = $(OBJDIR)/libproto.a
$(PROTO_LIB): $(PROTO_OBJS)
PROTO_LDFLAGS = $(shell $(PKG_CONFIG) --libs protobuf) -pthread $(PROTO_LIB)
.PRECIOUS: $(PROTO_HDRS) $(PROTO_SRCS)

include dep/agg/build.mk
include dep/libusbp/build.mk
include dep/stb/build.mk
include dep/emu/build.mk
include dep/fatfs/build.mk
include dep/adflib/build.mk
include dep/hfsutils/build.mk
include scripts/build.mk

include lib/build.mk
include arch/build.mk
include src/build.mk
include src/gui/build.mk
include tools/build.mk
include tests/build.mk

do-encodedecodetest = $(eval $(do-encodedecodetest-impl))
define do-encodedecodetest-impl

tests: $(OBJDIR)/$1$3.flux.encodedecode
$(OBJDIR)/$1$3.flux.encodedecode: scripts/encodedecodetest.sh $(FLUXENGINE_BIN) $2
	@mkdir -p $(dir $$@)
	@echo ENCODEDECODETEST $1 flux $(FLUXENGINE_BIN) $2 $3
	@scripts/encodedecodetest.sh $1 flux $(FLUXENGINE_BIN) $2 $3 > $$@

tests: $(OBJDIR)/$1$3.scp.encodedecode
$(OBJDIR)/$1$3.scp.encodedecode: scripts/encodedecodetest.sh $(FLUXENGINE_BIN) $2
	@mkdir -p $(dir $$@)
	@echo ENCODEDECODETEST $1 scp $(FLUXENGINE_BIN) $2 $3
	@scripts/encodedecodetest.sh $1 scp $(FLUXENGINE_BIN) $2 $3 > $$@

endef

$(call do-encodedecodetest,agat840)
$(call do-encodedecodetest,amiga)
$(call do-encodedecodetest,apple2,,--140)
$(call do-encodedecodetest,atarist,,--360)
$(call do-encodedecodetest,atarist,,--370)
$(call do-encodedecodetest,atarist,,--400)
$(call do-encodedecodetest,atarist,,--410)
$(call do-encodedecodetest,atarist,,--720)
$(call do-encodedecodetest,atarist,,--740)
$(call do-encodedecodetest,atarist,,--800)
$(call do-encodedecodetest,atarist,,--820)
$(call do-encodedecodetest,bk800)
$(call do-encodedecodetest,brother,,--120)
$(call do-encodedecodetest,brother,,--240)
$(call do-encodedecodetest,commodore1541,scripts/commodore1541_test.textpb,--171)
$(call do-encodedecodetest,commodore1541,scripts/commodore1541_test.textpb,--192)
$(call do-encodedecodetest,commodore1581)
$(call do-encodedecodetest,cmd_fd2000)
$(call do-encodedecodetest,hplif,,--264)
$(call do-encodedecodetest,hplif,,--616)
$(call do-encodedecodetest,hplif,,--770)
$(call do-encodedecodetest,ibm,,--1200)
$(call do-encodedecodetest,ibm,,--1232)
$(call do-encodedecodetest,ibm,,--1440)
$(call do-encodedecodetest,ibm,,--1680)
$(call do-encodedecodetest,ibm,,--180)
$(call do-encodedecodetest,ibm,,--160)
$(call do-encodedecodetest,ibm,,--320)
$(call do-encodedecodetest,ibm,,--360)
$(call do-encodedecodetest,ibm,,--720)
$(call do-encodedecodetest,mac,scripts/mac400_test.textpb,--400)
$(call do-encodedecodetest,mac,scripts/mac800_test.textpb,--800)
$(call do-encodedecodetest,n88basic)
$(call do-encodedecodetest,rx50)
$(call do-encodedecodetest,tids990)
$(call do-encodedecodetest,victor9k,,--612)
$(call do-encodedecodetest,victor9k,,--1224)

$(OBJDIR)/%.a:
	@mkdir -p $(dir $@)
	@echo AR $@
	@$(AR) rc $@ $^

%.exe:
	@mkdir -p $(dir $@)
	@echo LINK $@
	@$(CXX) -o $@ $^ $(LDFLAGS) $(LDFLAGS)

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo CXX $<
	@$(CXX) $(CFLAGS) $(CXXFLAGS) -MMD -MP -MF $(@:.o=.d) -c -o $@ $<

$(OBJDIR)/%.o: %.cc
	@mkdir -p $(dir $@)
	@echo CXX $<
	@$(CXX) $(CFLAGS) $(CXXFLAGS) -MMD -MP -MF $(@:.o=.d) -c -o $@ $<

$(OBJDIR)/%.o: $(OBJDIR)/%.cc
	@mkdir -p $(dir $@)
	@echo CXX $<
	@$(CXX) $(CFLAGS) $(CXXFLAGS) -MMD -MP -MF $(@:.o=.d) -c -o $@ $<

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo CC $<
	@$(CC) $(CFLAGS) $(CFLAGS) -MMD -MP -MF $(@:.o=.d) -c -o $@ $<

$(OBJDIR)/%.pb.h: %.proto
	@mkdir -p $(dir $@)
	@echo PROTOC $@
	@$(PROTOC) -I. --cpp_out=$(OBJDIR) $<

clean:
	rm -rf $(OBJDIR)

install: install-bin # install-man install-docs ...

install-bin: fluxengine$(EXT) fluxengine-gui$(EXT) brother120tool$(EXT) brother240tool$(EXT) upgrade-flux-file$(EXT)
	install -d "$(DESTDIR)$(BINDIR)"
	for target in $^; do \
		install $$target "$(DESTDIR)$(BINDIR)/$$target"; \
	done

-include $(OBJS:%.o=%.d)
