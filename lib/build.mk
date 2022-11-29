LIBFLUXENGINE_SRCS = \
	lib/bitmap.cc \
	lib/bytes.cc \
	lib/crc.cc \
	lib/csvreader.cc \
	lib/decoders/decoders.cc \
	lib/decoders/fluxdecoder.cc \
	lib/decoders/fluxmapreader.cc \
	lib/decoders/fmmfm.cc \
	lib/encoders/encoders.cc \
	lib/environment.cc \
	lib/flags.cc \
	lib/fluxmap.cc \
	lib/fluxsink/a2rfluxsink.cc \
	lib/fluxsink/aufluxsink.cc \
	lib/fluxsink/fl2fluxsink.cc \
	lib/fluxsink/fluxsink.cc \
	lib/fluxsink/hardwarefluxsink.cc \
	lib/fluxsink/scpfluxsink.cc \
	lib/fluxsink/vcdfluxsink.cc \
	lib/fluxsource/cwffluxsource.cc \
	lib/fluxsource/erasefluxsource.cc \
	lib/fluxsource/fl2fluxsource.cc \
	lib/fluxsource/fluxsource.cc \
	lib/fluxsource/flx.cc \
	lib/fluxsource/flxfluxsource.cc \
	lib/fluxsource/hardwarefluxsource.cc \
	lib/fluxsource/kryoflux.cc \
	lib/fluxsource/kryofluxfluxsource.cc \
	lib/fluxsource/memoryfluxsource.cc \
	lib/fluxsource/scpfluxsource.cc \
	lib/fluxsource/testpatternfluxsource.cc \
	lib/globals.cc \
	lib/hexdump.cc \
	lib/image.cc \
	lib/imagereader/d64imagereader.cc \
	lib/imagereader/d88imagereader.cc \
	lib/imagereader/dimimagereader.cc \
	lib/imagereader/diskcopyimagereader.cc \
	lib/imagereader/fdiimagereader.cc \
	lib/imagereader/imagereader.cc \
	lib/imagereader/imdimagereader.cc \
	lib/imagereader/imgimagereader.cc \
	lib/imagereader/jv3imagereader.cc \
	lib/imagereader/nfdimagereader.cc \
	lib/imagereader/nsiimagereader.cc \
	lib/imagereader/td0imagereader.cc \
	lib/imagewriter/d64imagewriter.cc \
	lib/imagewriter/d88imagewriter.cc \
	lib/imagewriter/diskcopyimagewriter.cc \
	lib/imagewriter/imagewriter.cc \
	lib/imagewriter/imdimagewriter.cc \
	lib/imagewriter/imgimagewriter.cc \
	lib/imagewriter/ldbsimagewriter.cc \
	lib/imagewriter/nsiimagewriter.cc \
	lib/imagewriter/rawimagewriter.cc \
	lib/layout.cc \
	lib/ldbs.cc \
	lib/logger.cc \
	lib/proto.cc \
	lib/readerwriter.cc \
	lib/sector.cc \
	lib/usb/fluxengineusb.cc \
	lib/usb/greaseweazle.cc \
	lib/usb/greaseweazleusb.cc \
	lib/usb/serial.cc \
	lib/usb/usb.cc \
	lib/usb/usbfinder.cc \
	lib/utils.cc \
	lib/vfs/acorndfs.cc \
	lib/vfs/amigaffs.cc \
	lib/vfs/applesingle.cc \
	lib/vfs/brother120fs.cc \
	lib/vfs/cbmfs.cc \
	lib/vfs/cpmfs.cc \
	lib/vfs/fatfs.cc \
	lib/vfs/machfs.cc \
	lib/vfs/prodos.cc \
	lib/vfs/smaky6fs.cc \
	lib/vfs/vfs.cc \
	lib/vfs/fluxsectorinterface.cc \
	lib/vfs/imagesectorinterface.cc \

LIBFLUXENGINE_OBJS = $(patsubst %.cc, $(OBJDIR)/%.o, $(LIBFLUXENGINE_SRCS))
OBJS += $(LIBFLUXENGINE_OBJS)
$(LIBFLUXENGINE_SRCS): | $(PROTO_HDRS)
LIBFLUXENGINE_LIB = $(OBJDIR)/libfluxengine.a
LIBFLUXENGINE_CFLAGS = 
LIBFLUXENGINE_LDFLAGS = $(LIBFLUXENGINE_LIB)

$(LIBFLUXENGINE_LIB): $(LIBFLUXENGINE_OBJS)

$(LIBFLUXENGINE_OBJS): CFLAGS += $(LIBARCH_CFLAGS)
$(LIBFLUXENGINE_OBJS): CFLAGS += $(LIBUSBP_CFLAGS)
$(LIBFLUXENGINE_OBJS): CFLAGS += $(PROTO_CFLAGS)
$(LIBFLUXENGINE_OBJS): CFLAGS += $(FATFS_CFLAGS)
$(LIBFLUXENGINE_OBJS): CFLAGS += $(ADFLIB_CFLAGS)
$(LIBFLUXENGINE_OBJS): CFLAGS += $(HFSUTILS_CFLAGS)

$(call use-pkgconfig, $(LIBFLUXENGINE_LIB), $(LIBFLUXENGINE_OBJS), fmt)
