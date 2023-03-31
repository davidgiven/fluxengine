include src/formats/build.mk

FLUXENGINE_SRCS = \
	src/fe-analysedriveresponse.cc \
	src/fe-analyselayout.cc \
	src/fe-format.cc \
	src/fe-getdiskinfo.cc \
	src/fe-getfile.cc \
	src/fe-getfileinfo.cc \
	src/fe-inspect.cc \
	src/fe-ls.cc \
	src/fe-merge.cc \
	src/fe-mkdir.cc \
	src/fe-mv.cc \
	src/fe-rm.cc \
	src/fe-putfile.cc \
	src/fe-rawread.cc \
	src/fe-rawwrite.cc \
	src/fe-read.cc \
	src/fe-rpm.cc \
	src/fe-seek.cc \
	src/fe-testbandwidth.cc \
	src/fe-testvoltages.cc \
	src/fe-write.cc \
	src/fileutils.cc \
	src/fluxengine.cc \
 
FLUXENGINE_OBJS = $(patsubst %.cc, $(OBJDIR)/%.o, $(FLUXENGINE_SRCS))
OBJS += $(FLUXENGINE_OBJS)
$(FLUXENGINE_SRCS): | $(PROTO_HDRS)
FLUXENGINE_BIN = $(OBJDIR)/fluxengine.exe
$(FLUXENGINE_BIN): $(FLUXENGINE_OBJS)

$(call use-pkgconfig, $(FLUXENGINE_BIN), $(FLUXENGINE_OBJS), fmt)
$(call use-library, $(FLUXENGINE_BIN), $(FLUXENGINE_OBJS), AGG)
$(call use-library, $(FLUXENGINE_BIN), $(FLUXENGINE_OBJS), LIBARCH)
$(call use-library, $(FLUXENGINE_BIN), $(FLUXENGINE_OBJS), LIBFLUXENGINE)
$(call use-library, $(FLUXENGINE_BIN), $(FLUXENGINE_OBJS), LIBFORMATS)
$(call use-library, $(FLUXENGINE_BIN), $(FLUXENGINE_OBJS), LIBUSBP)
$(call use-library, $(FLUXENGINE_BIN), $(FLUXENGINE_OBJS), PROTO)
$(call use-library, $(FLUXENGINE_BIN), $(FLUXENGINE_OBJS), STB)
$(call use-library, $(FLUXENGINE_BIN), $(FLUXENGINE_OBJS), FATFS)
$(call use-library, $(FLUXENGINE_BIN), $(FLUXENGINE_OBJS), ADFLIB)
$(call use-library, $(FLUXENGINE_BIN), $(FLUXENGINE_OBJS), HFSUTILS)

binaries: fluxengine$(EXT)

fluxengine$(EXT): $(FLUXENGINE_BIN)
	@echo CP $@
	@cp $< $@


