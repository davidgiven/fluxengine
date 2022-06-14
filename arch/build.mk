LIBARCH_SRCS = \
	arch/aeslanier/decoder.cc \
	arch/amiga/amiga.cc \
	arch/amiga/decoder.cc \
	arch/amiga/encoder.cc \
	arch/apple2/decoder.cc \
	arch/apple2/encoder.cc \
	arch/brother/decoder.cc \
	arch/brother/encoder.cc \
	arch/c64/c64.cc \
	arch/c64/decoder.cc \
	arch/c64/encoder.cc \
	arch/f85/decoder.cc \
	arch/fb100/decoder.cc \
	arch/ibm/decoder.cc \
	arch/ibm/encoder.cc \
	arch/macintosh/decoder.cc \
	arch/macintosh/encoder.cc \
	arch/mx/decoder.cc \
	arch/victor9k/decoder.cc \
	arch/victor9k/encoder.cc \
	arch/zilogmcz/decoder.cc \
	arch/tids990/decoder.cc \
	arch/tids990/encoder.cc \
	arch/micropolis/decoder.cc \
	arch/micropolis/encoder.cc \
	arch/northstar/decoder.cc \
	arch/northstar/encoder.cc \
	arch/agat/agat.cc \
	arch/agat/decoder.cc \

LIBARCH_OBJS = $(patsubst %.cc, $(OBJDIR)/%.o, $(LIBARCH_SRCS))
$(LIBARCH_SRCS): | $(PROTO_HDRS)
$(LIBARCH_SRCS): CFLAGS += $(PROTO_CFLAGS)
LIBARCH_LIB = $(OBJDIR)/libarch.a
$(LIBARCH_LIB): $(LIBARCH_OBJS)

LIBARCH_LDFLAGS = $(LIBARCH_LIB)

$(call use-pkgconfig, $(LIBARCH_LIB), $(LIBARCH_OBJS), fmt)
