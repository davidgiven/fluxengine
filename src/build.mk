include src/formats/build.mk

FLUXENGINE_SRCS = \
	src/fe-analysedriveresponse.cc \
	src/fe-analyselayout.cc \
	src/fe-inspect.cc \
	src/fe-rawread.cc \
	src/fe-rawwrite.cc \
	src/fe-read.cc \
	src/fe-rpm.cc \
	src/fe-seek.cc \
	src/fe-testbandwidth.cc \
	src/fe-testvoltages.cc \
	src/fe-write.cc \
	src/fluxengine.cc \
 
FLUXENGINE_OBJS = $(patsubst %.cc, $(OBJDIR)/%.o, $(FLUXENGINE_SRCS))
$(FLUXENGINE_SRCS): | $(PROTO_HDRS)
fluxengine.exe: $(FLUXENGINE_OBJS)

$(call use-pkgconfig, fluxengine.exe, $(FLUXENGINE_OBJS), fmt)
$(call use-library, fluxengine.exe, $(FLUXENGINE_OBJS), AGG)
$(call use-library, fluxengine.exe, $(FLUXENGINE_OBJS), LIBARCH)
$(call use-library, fluxengine.exe, $(FLUXENGINE_OBJS), LIBFLUXENGINE)
$(call use-library, fluxengine.exe, $(FLUXENGINE_OBJS), LIBFORMATS)
$(call use-library, fluxengine.exe, $(FLUXENGINE_OBJS), LIBUSBP)
$(call use-library, fluxengine.exe, $(FLUXENGINE_OBJS), PROTO)
$(call use-library, fluxengine.exe, $(FLUXENGINE_OBJS), STB)

all: fluxengine.exe

