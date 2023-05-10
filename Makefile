#Special Windows settings.

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

#Required to get the gcc run - time libraries on the path.
	export PATH := $(PATH):$(MINGWBIN)
	EXT ?= .exe
endif

#Special OSX settings.

ifeq ($(shell uname),Darwin)
	PLATFORM = OSX
	LDFLAGS += \
		-framework IOKit \
		-framework Foundation
endif

#Check the Make version.


ifeq ($(findstring 4.,$(MAKE_VERSION)),)
$(error You need GNU Make 4.x for this (if you're on OSX, use gmake).)
endif

#Normal settings.

OBJDIR ?= .obj
CCPREFIX ?=
LUA ?= lua
CC ?= $(CCPREFIX)gcc
CXX ?= $(CCPREFIX)g++
AR ?= $(CCPREFIX)ar
PKG_CONFIG ?= pkg-config
WX_CONFIG ?= wx-config
PROTOC ?= protoc
CFLAGS ?= -g -O3
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

empty :=
space := $(empty) $(empty)

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
all: binaries tests docs

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

tests: $(OBJDIR)/$1$$(subst $$(space),_,$3).flux.encodedecode
$(OBJDIR)/$1$$(subst $$(space),_,$3).flux.encodedecode: scripts/encodedecodetest.sh $(FLUXENGINE_BIN) $2
	@mkdir -p $(dir $$@)
	@echo ENCODEDECODETEST $1 flux $(FLUXENGINE_BIN) $2 $3
	@scripts/encodedecodetest.sh $1 flux $(FLUXENGINE_BIN) $2 $3 > $$@

tests: $(OBJDIR)/$1$$(subst $$(space),_,$3).scp.encodedecode
$(OBJDIR)/$1$$(subst $$(space),_,$3).scp.encodedecode: scripts/encodedecodetest.sh $(FLUXENGINE_BIN) $2
	@mkdir -p $(dir $$@)
	@echo ENCODEDECODETEST $1 scp $(FLUXENGINE_BIN) $2 $3
	@scripts/encodedecodetest.sh $1 scp $(FLUXENGINE_BIN) $2 $3 > $$@

endef

$(call do-encodedecodetest,agat,,--drive.tpi=96)
$(call do-encodedecodetest,amiga,,--drive.tpi=135)
$(call do-encodedecodetest,apple2,,--140 --drive.tpi=96)
$(call do-encodedecodetest,atarist,,--360 --drive.tpi=135)
$(call do-encodedecodetest,atarist,,--370 --drive.tpi=135)
$(call do-encodedecodetest,atarist,,--400 --drive.tpi=135)
$(call do-encodedecodetest,atarist,,--410 --drive.tpi=135)
$(call do-encodedecodetest,atarist,,--720 --drive.tpi=135)
$(call do-encodedecodetest,atarist,,--740 --drive.tpi=135)
$(call do-encodedecodetest,atarist,,--800 --drive.tpi=135)
$(call do-encodedecodetest,atarist,,--820 --drive.tpi=135)
$(call do-encodedecodetest,bk)
$(call do-encodedecodetest,brother,,--120 --drive.tpi=135)
$(call do-encodedecodetest,brother,,--240 --drive.tpi=135)
$(call do-encodedecodetest,commodore,scripts/commodore1541_test.textpb,--171 --drive.tpi=96)
$(call do-encodedecodetest,commodore,scripts/commodore1541_test.textpb,--192 --drive.tpi=96)
$(call do-encodedecodetest,commodore,,--800 --drive.tpi=135)
$(call do-encodedecodetest,commodore,,--1620 --drive.tpi=135)
$(call do-encodedecodetest,hplif,,--264 --drive.tpi=135)
$(call do-encodedecodetest,hplif,,--616 --drive.tpi=135)
$(call do-encodedecodetest,hplif,,--770 --drive.tpi=135)
$(call do-encodedecodetest,ibm,,--1200 --drive.tpi=96)
$(call do-encodedecodetest,ibm,,--1232 --drive.tpi=96)
$(call do-encodedecodetest,ibm,,--1440 --drive.tpi=135)
$(call do-encodedecodetest,ibm,,--1680 --drive.tpi=135)
$(call do-encodedecodetest,ibm,,--180 --drive.tpi=96)
$(call do-encodedecodetest,ibm,,--160 --drive.tpi=96)
$(call do-encodedecodetest,ibm,,--320 --drive.tpi=96)
$(call do-encodedecodetest,ibm,,--360 --drive.tpi=96)
$(call do-encodedecodetest,ibm,,--720_96 --drive.tpi=96)
$(call do-encodedecodetest,ibm,,--720_135 --drive.tpi=135)
$(call do-encodedecodetest,mac,scripts/mac400_test.textpb,--400 --drive.tpi=135)
$(call do-encodedecodetest,mac,scripts/mac800_test.textpb,--800 --drive.tpi=135)
$(call do-encodedecodetest,n88basic,,--drive.tpi=96)
$(call do-encodedecodetest,rx50,,--drive.tpi=96)
$(call do-encodedecodetest,tids990,,--drive.tpi=48)
$(call do-encodedecodetest,victor9k,,--612 --drive.tpi=96)
$(call do-encodedecodetest,victor9k,,--1224 --drive.tpi=96)

do-corpustest = $(eval $(do-corpustest-impl))
define do-corpustest-impl

tests: $(OBJDIR)/corpustest/$2
$(OBJDIR)/corpustest/$2: $(FLUXENGINE_BIN) \
		../fluxengine-testdata/data/$1 ../fluxengine-testdata/data/$2
	@mkdir -p $(OBJDIR)/corpustest
	@echo CORPUSTEST $1 $2 $3
	@$(FLUXENGINE_BIN) read $3 -s ../fluxengine-testdata/data/$1 -o $$@ > $$@.log
	@cmp $$@ ../fluxengine-testdata/data/$2

endef

ifneq ($(wildcard ../fluxengine-testdata/data),)

$(call do-corpustest,amiga.flux,amiga.adf,amiga --drive.tpi=135)
$(call do-corpustest,atarist360.flux,atarist360.st,atarist --360 --drive.tpi=135)
$(call do-corpustest,atarist720.flux,atarist720.st,atarist --720 --drive.tpi=135)
$(call do-corpustest,brother120.flux,brother120.img,brother --120 --drive.tpi=135)
$(call do-corpustest,cmd-fd2000.flux,cmd-fd2000.img,commodore --1620 --drive.tpi=135)
$(call do-corpustest,ibm1232.flux,ibm1232.img,ibm --1232 --drive.tpi=96)
$(call do-corpustest,ibm1440.flux,ibm1440.img,ibm --1440 --drive.tpi=135)
$(call do-corpustest,mac800.flux,mac800.dsk,mac --800 --drive.tpi=135)
$(call do-corpustest,micropolis315.flux,micropolis315.img,micropolis --315 --drive.tpi=100)
$(call do-corpustest,northstar87-synthetic.flux,northstar87-synthetic.nsi,northstar --87 --drive.tpi=48)
$(call do-corpustest,northstar175-synthetic.flux,northstar175-synthetic.nsi,northstar --175 --drive.tpi=48)
$(call do-corpustest,northstar350-synthetic.flux,northstar350-synthetic.nsi,northstar --350 --drive.tpi=48)
$(call do-corpustest,victor9k_ss.flux,victor9k_ss.img,victor9k --612 --drive.tpi=96)
$(call do-corpustest,victor9k_ds.flux,victor9k_ds.img,victor9k --1224 --drive.tpi=96)

endif

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
