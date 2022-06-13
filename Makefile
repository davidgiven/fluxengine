# Special Windows settings.

ifeq ($(OS), Windows_NT)
	MINGWBIN = /mingw32/bin
	CCPREFIX = $(MINGWBIN)/
	LUA = $(MINGWBIN)/lua
	PKG_CONFIG = $(MINGWBIN)/pkg-config
	WX_CONFIG = /usr/bin/sh $(MINGWBIN)/wx-config
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

use-library = $(eval $(use-library-impl))
define use-library-impl
$(1): | $(call $(3)_LIB)
$(1): private LDFLAGS += $(call $(3)_LDFLAGS)
$(2): private CFLAGS += $(call $(3)_CFLAGS)
endef

all: fluxengine.exe

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
PROTO_CFLAGS = $(shell pkg-config --cflags protobuf)
$(PROTO_SRCS): | $(PROTO_HDRS)
$(PROTO_OBJS): CFLAGS += $(PROTO_CFLAGS)
PROTO_LIB = $(OBJDIR)/libproto.a
$(PROTO_LIB): $(PROTO_OBJS)
PROTO_LDFLAGS = $(shell pkg-config --libs protobuf) -pthread $(PROTO_LIB)
.PRECIOUS: $(PROTO_HDRS) $(PROTO_SRCS)

include dep/agg/build.mk
include dep/libusbp/build.mk
include dep/stb/build.mk
include dep/fmt/build.mk

include lib/build.mk
include arch/build.mk
include src/build.mk
include src/gui/build.mk

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

#PACKAGES = zlib sqlite3 protobuf
#
#export CFLAGS = \
#	-ffunction-sections -fdata-sections
#export CXXFLAGS = $(CFLAGS) \
#	-x c++ --std=gnu++2a \
#	-Wno-deprecated-enum-enum-conversion \
#	-Wno-deprecated-enum-float-conversion
#export LDFLAGS = -pthread
#
#export COPTFLAGS = -Os
#export LDOPTFLAGS = -Os
#
#export CDBGFLAGS = -O0 -g
#export LDDBGFLAGS = -O0 -g
#
#ifeq ($(OS), Windows_NT)
#else
#ifeq ($(shell uname),Darwin)
#else
#	PACKAGES += libudev
#endif
#endif
#
#ifeq ($(OS), Windows_NT)
#export PROTOC = /mingw32/bin/protoc
#export CC = /mingw32/bin/gcc
#export CXX = /mingw32/bin/g++
#export AR = /mingw32/bin/ar rc
#export RANLIB = /mingw32/bin/ranlib
#export STRIP = /mingw32/bin/strip
#export CFLAGS += -I/mingw32/include
#export CXXFLAGS += $(shell wx-config --cxxflags --static=yes)
#export GUILDFLAGS += -lmingw32
#export LIBS += -L/mingw32/lib -static\
#	-lsqlite3 -lz \
#	-lsetupapi -lwinusb -lole32 -lprotobuf -luuid
#export GUILIBS += -L/mingw32/lib -static -lsqlite3 \
#	$(shell wx-config --libs --static=yes core base) -lz \
#	-lcomctl32 -loleaut32 -lspoolss -loleacc -lwinspool \
#	-lsetupapi -lwinusb -lole32 -lprotobuf -luuid
#export EXTENSION = .exe
#else
#
#packages-exist = $(shell pkg-config --exists $(PACKAGES) && echo yes)
#ifneq ($(packages-exist),yes)
#$(warning These pkg-config packages are installed: $(shell pkg-config --list-all | sort | awk '{print $$1}'))
#$(error You must have these pkg-config packages installed: $(PACKAGES))
#endif
#wx-exist = $(shell wx-config --cflags > /dev/null && echo yes)
#ifneq ($(wx-exist),yes)
#$(error You must have these wx-config installed)
#endif
#
#export PROTOC = protoc
#export CC = gcc
#export CXX = g++
#export AR = ar rc
#export RANLIB = ranlib
#export STRIP = strip
#export CFLAGS += $(shell pkg-config --cflags $(PACKAGES)) $(shell wx-config --cxxflags)
#export LDFLAGS +=
#export LIBS += $(shell pkg-config --libs $(PACKAGES))
#export GUILIBS += $(shell wx-config --libs core base)
#export EXTENSION =
#
#ifeq ($(shell uname),Darwin)
#AR = ar rcS
#RANLIB += -c -no_warning_for_no_symbols
#export CC = clang
#export CXX = clang++
#export COBJC = clang
#export LDFLAGS += -framework IOKit -framework CoreFoundation
#export CFLAGS += -Wno-deprecated-declarations
#endif
#
#endif
#export XXD = xxd
#
#CFLAGS += -Ilib -Idep/fmt -Iarch
#
#export OBJDIR = .obj
#
#all: .obj/build.ninja
#	@ninja -f .obj/build.ninja
#	@if command -v cscope > /dev/null; then cscope -bRq; fi
#
#clean:
#	@echo CLEAN
#	@rm -rf $(OBJDIR)
#
#.obj/build.ninja: mkninja.sh Makefile
#	@echo MKNINJA $@
#	@mkdir -p $(OBJDIR)
#	@sh $< > $@
#
#compdb:
#	@ninja -f .obj/build.ninja -t compdb > compile_commands.json
