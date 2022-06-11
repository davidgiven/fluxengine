vars.cflags = { "$(CFLAGS)" }
vars.cxxflags = { "$(CXXFLAGS)" }
vars.ldflags = { "-pthread" }

include "build/protobuf.lua"
include "build/dependency.lua"

dependency {
	name = "fmt_dep",
	pkg_config = "fmt",
	fallback = "dep/fmt+fmt"
}

dependency {
	name = "protobuf_dep",
	pkg_config = "protobuf"
}

dependency {
	name = "zlib_dep",
	pkg_config = "zlib"
}

proto_cc_library {
	name = "config_lib",
	srcs = {
		"./lib/common.proto",
		"./lib/config.proto",
		"./lib/decoders/decoders.proto",
		"./lib/drive.proto",
		"./lib/encoders/encoders.proto",
		"./lib/fl2.proto",
		"./lib/fluxsink/fluxsink.proto",
		"./lib/fluxsource/fluxsource.proto",
		"./lib/imagereader/imagereader.proto",
		"./lib/imagewriter/imagewriter.proto",
		"./lib/mapper.proto",
		"./lib/usb/usb.proto",
		"./arch/aeslanier/aeslanier.proto",
		"./arch/agat/agat.proto",
		"./arch/amiga/amiga.proto",
		"./arch/apple2/apple2.proto",
		"./arch/brother/brother.proto",
		"./arch/c64/c64.proto",
		"./arch/f85/f85.proto",
		"./arch/fb100/fb100.proto",
		"./arch/ibm/ibm.proto",
		"./arch/macintosh/macintosh.proto",
		"./arch/micropolis/micropolis.proto",
		"./arch/mx/mx.proto",
		"./arch/northstar/northstar.proto",
		"./arch/tids990/tids990.proto",
		"./arch/victor9k/victor9k.proto",
		"./arch/zilogmcz/zilogmcz.proto",
	}
}

clibrary {
	name = "protocol_lib",
	hdrs = { "protocol.h" }
}

clibrary {
	name = "libfluxengine",
	srcs = {
		"./arch/aeslanier/decoder.cc",
		"./arch/agat/agat.cc",
		"./arch/agat/decoder.cc",
		"./arch/amiga/amiga.cc",
		"./arch/amiga/decoder.cc",
		"./arch/amiga/encoder.cc",
		"./arch/apple2/decoder.cc",
		"./arch/apple2/encoder.cc",
		"./arch/brother/decoder.cc",
		"./arch/brother/encoder.cc",
		"./arch/c64/c64.cc",
		"./arch/c64/decoder.cc",
		"./arch/c64/encoder.cc",
		"./arch/f85/decoder.cc",
		"./arch/fb100/decoder.cc",
		"./arch/ibm/decoder.cc",
		"./arch/ibm/encoder.cc",
		"./arch/macintosh/decoder.cc",
		"./arch/macintosh/encoder.cc",
		"./arch/micropolis/decoder.cc",
		"./arch/micropolis/encoder.cc",
		"./arch/mx/decoder.cc",
		"./arch/northstar/decoder.cc",
		"./arch/northstar/encoder.cc",
		"./arch/tids990/decoder.cc",
		"./arch/tids990/encoder.cc",
		"./arch/victor9k/decoder.cc",
		"./arch/victor9k/encoder.cc",
		"./arch/zilogmcz/decoder.cc",
		"./lib/bitmap.cc",
		"./lib/bytes.cc",
		"./lib/crc.cc",
		"./lib/csvreader.cc",
		"./lib/decoders/decoders.cc",
		"./lib/decoders/fluxdecoder.cc",
		"./lib/decoders/fluxmapreader.cc",
		"./lib/decoders/fmmfm.cc",
		"./lib/encoders/encoders.cc",
		"./lib/flags.cc",
		"./lib/fluxmap.cc",
		"./lib/fluxsink/aufluxsink.cc",
		"./lib/fluxsink/fl2fluxsink.cc",
		"./lib/fluxsink/fluxsink.cc",
		"./lib/fluxsink/hardwarefluxsink.cc",
		"./lib/fluxsink/scpfluxsink.cc",
		"./lib/fluxsink/vcdfluxsink.cc",
		"./lib/fluxsource/cwffluxsource.cc",
		"./lib/fluxsource/erasefluxsource.cc",
		"./lib/fluxsource/fl2fluxsource.cc",
		"./lib/fluxsource/fluxsource.cc",
		"./lib/fluxsource/hardwarefluxsource.cc",
		"./lib/fluxsource/kryoflux.cc",
		"./lib/fluxsource/kryofluxfluxsource.cc",
		"./lib/fluxsource/scpfluxsource.cc",
		"./lib/fluxsource/testpatternfluxsource.cc",
		"./lib/globals.cc",
		"./lib/hexdump.cc",
		"./lib/image.cc",
		"./lib/imagereader/d64imagereader.cc",
		"./lib/imagereader/d88imagereader.cc",
		"./lib/imagereader/dimimagereader.cc",
		"./lib/imagereader/diskcopyimagereader.cc",
		"./lib/imagereader/fdiimagereader.cc",
		"./lib/imagereader/imagereader.cc",
		"./lib/imagereader/imdimagereader.cc",
		"./lib/imagereader/imgimagereader.cc",
		"./lib/imagereader/jv3imagereader.cc",
		"./lib/imagereader/nfdimagereader.cc",
		"./lib/imagereader/nsiimagereader.cc",
		"./lib/imagereader/td0imagereader.cc",
		"./lib/imagewriter/d64imagewriter.cc",
		"./lib/imagewriter/d88imagewriter.cc",
		"./lib/imagewriter/diskcopyimagewriter.cc",
		"./lib/imagewriter/imagewriter.cc",
		"./lib/imagewriter/imgimagewriter.cc",
		"./lib/imagewriter/ldbsimagewriter.cc",
		"./lib/imagewriter/nsiimagewriter.cc",
		"./lib/imagewriter/rawimagewriter.cc",
		"./lib/imginputoutpututils.cc",
		"./lib/ldbs.cc",
		"./lib/logger.cc",
		"./lib/mapper.cc",
		"./lib/proto.cc",
		"./lib/readerwriter.cc",
		"./lib/sector.cc",
		"./lib/usb/fluxengineusb.cc",
		"./lib/usb/greaseweazle.cc",
		"./lib/usb/greaseweazleusb.cc",
		"./lib/usb/serial.cc",
		"./lib/usb/usb.cc",
		"./lib/usb/usbfinder.cc",
		"./lib/utils.cc",
		"protocol.h",
	},
	deps = {
		"+config_lib",
		"+protocol_lib",
		"+fmt_dep",
		"+protobuf_dep",
		"+zlib_dep",
		"dep/libusbp+libusbp",
	},
	vars = {
		["+cflags"] = { "-Ilib", "-Iarch", "-I." }
	}
}

installable {
	name = "all",
	map = {
		["fluxengine"] = "src+fluxengine",
		["fluxengine-gui"] = "src/gui+fluxengine",
	}
}

