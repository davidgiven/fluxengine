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
endif

# Special OSX settings.

ifeq ($(shell uname),Darwin)
	PLATFORM = OSX
	LDFLAGS += \
		-framework IOKit \
		-framework Foundation
endif

# Normal settings.

OBJDIR ?= .obj
CCPREFIX ?=
LUA ?= lua
CC = $(CCPREFIX)gcc
CXX = $(CCPREFIX)g++
AR = $(CCPREFIX)ar
PKG_CONFIG ?= pkg-config
WX_CONFIG ?= wx-config
PROTOC ?= protoc
CFLAGS ?= -g -Os
CXXFLAGS ?= -std=c++17 \
	-Wno-deprecated-enum-float-conversion \
	-Wno-deprecated-enum-enum-conversion
LDFLAGS ?=
PLATFORM ?= UNIX
TESTS ?= yes

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
$(1): | $(call $(3)_LIB)
$(1): private LDFLAGS += $(call $(3)_LDFLAGS)
$(2): private CFLAGS += $(call $(3)_CFLAGS)
endef

use-pkgconfig = $(eval $(use-pkgconfig-impl))
define use-pkgconfig-impl
ifneq ($(strip $(shell $(PKG_CONFIG) $3; echo $$?)),0)
$$(error Missing required pkg-config dependency: $3)
endif

$(1): private LDFLAGS += $(shell $(PKG_CONFIG) --libs $(3))
$(2): private CFLAGS += $(shell $(PKG_CONFIG) --cflags $(3))
endef

all: tests

PROTOS = \
	arch/aeslanier/aeslanier.proto \
	arch/amiga/amiga.proto \
	arch/apple2/apple2.proto \
	arch/brother/brother.proto \
	arch/c64/c64.proto \
	arch/f85/f85.proto \
	arch/fb100/fb100.proto \
	arch/ibm/ibm.proto \
	arch/macintosh/macintosh.proto \
	arch/mx/mx.proto \
	arch/victor9k/victor9k.proto \
	arch/zilogmcz/zilogmcz.proto \
	arch/tids990/tids990.proto \
	arch/micropolis/micropolis.proto \
	arch/northstar/northstar.proto \
	arch/agat/agat.proto \
	lib/decoders/decoders.proto \
	lib/encoders/encoders.proto \
	lib/fluxsink/fluxsink.proto \
	lib/fluxsource/fluxsource.proto \
	lib/imagereader/imagereader.proto \
	lib/imagewriter/imagewriter.proto \
	lib/usb/usb.proto \
	lib/common.proto \
	lib/fl2.proto \
	lib/config.proto \
	lib/mapper.proto \
	lib/drive.proto \
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
include dep/fmt/build.mk
include dep/emu/build.mk

include lib/build.mk
include arch/build.mk
include src/build.mk
include src/gui/build.mk
include tools/build.mk
include tests/build.mk

do-encodedecodetest = $(eval $(do-encodedecodetest-impl))
define do-encodedecodetest-impl

tests: $(OBJDIR)/$1.encodedecode
$(OBJDIR)/$1.encodedecode: scripts/encodedecodetest.sh fluxengine.exe $2
	@mkdir -p $(dir $$@)
	@echo ENCODEDECODETEST $1
	@scripts/encodedecodetest.sh $1 flux $2 > $$@

endef

$(call do-encodedecodetest,amiga)
$(call do-encodedecodetest,apple2)
$(call do-encodedecodetest,atarist360)
$(call do-encodedecodetest,atarist370)
$(call do-encodedecodetest,atarist400)
$(call do-encodedecodetest,atarist410)
$(call do-encodedecodetest,atarist720)
$(call do-encodedecodetest,atarist740)
$(call do-encodedecodetest,atarist800)
$(call do-encodedecodetest,atarist820)
$(call do-encodedecodetest,bk800)
$(call do-encodedecodetest,brother120)
$(call do-encodedecodetest,brother240)
$(call do-encodedecodetest,commodore1541,scripts/commodore1541_test.textpb)
$(call do-encodedecodetest,commodore1581)
$(call do-encodedecodetest,hp9121)
$(call do-encodedecodetest,ibm1200)
$(call do-encodedecodetest,ibm1232)
$(call do-encodedecodetest,ibm1440)
$(call do-encodedecodetest,ibm180)
$(call do-encodedecodetest,ibm360)
$(call do-encodedecodetest,ibm720)
$(call do-encodedecodetest,mac400,scripts/mac400_test.textpb)
$(call do-encodedecodetest,mac800,scripts/mac800_test.textpb)
$(call do-encodedecodetest,n88basic)
$(call do-encodedecodetest,rx50)
$(call do-encodedecodetest,tids990)
$(call do-encodedecodetest,victor9k_ss)
$(call do-encodedecodetest,victor9k_ds)

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

-include $(LIB_OBJS:%.o=%.d)

