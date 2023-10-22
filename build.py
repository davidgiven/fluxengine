from build.ab import export
from build.c import clibrary
from build.protobuf import proto, protocc
from build.pkg import package

package(name="protobuf_lib", package="protobuf")
package(name="z_lib", package="zlib")
package(name="fmt_lib", package="fmt")
package(name="sqlite3_lib", package="sqlite3")

clibrary(name="protocol", hdrs={"protocol.h": "./protocol.h"})

proto(name="fl2_proto", srcs=["lib/fl2.proto"])
protocc(name="fl2_proto_lib", srcs=["+fl2_proto"])

clibrary(
    name="lib",
    srcs=[
        "./lib/bitmap.cc",
        "./lib/bytes.cc",
        "./lib/config.cc",
        "./lib/crc.cc",
        "./lib/csvreader.cc",
        "./lib/decoders/decoders.cc",
        "./lib/decoders/fluxdecoder.cc",
        "./lib/decoders/fluxmapreader.cc",
        "./lib/decoders/fmmfm.cc",
        "./lib/encoders/encoders.cc",
        "./lib/fl2.cc",
        "./lib/flags.cc",
        "./lib/fluxmap.cc",
        "./lib/fluxsink/a2rfluxsink.cc",
        "./lib/fluxsink/aufluxsink.cc",
        "./lib/fluxsink/fl2fluxsink.cc",
        "./lib/fluxsink/fluxsink.cc",
        "./lib/fluxsink/hardwarefluxsink.cc",
        "./lib/fluxsink/scpfluxsink.cc",
        "./lib/fluxsink/vcdfluxsink.cc",
        "./lib/fluxsource/a2rfluxsource.cc",
        "./lib/fluxsource/cwffluxsource.cc",
        "./lib/fluxsource/erasefluxsource.cc",
        "./lib/fluxsource/fl2fluxsource.cc",
        "./lib/fluxsource/fluxsource.cc",
        "./lib/fluxsource/flx.cc",
        "./lib/fluxsource/flxfluxsource.cc",
        "./lib/fluxsource/hardwarefluxsource.cc",
        "./lib/fluxsource/kryoflux.cc",
        "./lib/fluxsource/kryofluxfluxsource.cc",
        "./lib/fluxsource/memoryfluxsource.cc",
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
        "./lib/imagewriter/imdimagewriter.cc",
        "./lib/imagewriter/imgimagewriter.cc",
        "./lib/imagewriter/ldbsimagewriter.cc",
        "./lib/imagewriter/nsiimagewriter.cc",
        "./lib/imagewriter/rawimagewriter.cc",
        "./lib/layout.cc",
        "./lib/ldbs.cc",
        "./lib/logger.cc",
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
        "./lib/vfs/acorndfs.cc",
        "./lib/vfs/amigaffs.cc",
        "./lib/vfs/appledos.cc",
        "./lib/vfs/applesingle.cc",
        "./lib/vfs/brother120fs.cc",
        "./lib/vfs/cbmfs.cc",
        "./lib/vfs/cpmfs.cc",
        "./lib/vfs/fatfs.cc",
        "./lib/vfs/fluxsectorinterface.cc",
        "./lib/vfs/imagesectorinterface.cc",
        "./lib/vfs/lif.cc",
        "./lib/vfs/machfs.cc",
        "./lib/vfs/microdos.cc",
        "./lib/vfs/philefs.cc",
        "./lib/vfs/prodos.cc",
        "./lib/vfs/roland.cc",
        "./lib/vfs/smaky6fs.cc",
        "./lib/vfs/vfs.cc",
        "./lib/vfs/zdos.cc",
        "./arch/aeslanier/decoder.cc",
        "./arch/agat/agat.cc",
        "./arch/agat/decoder.cc",
        "./arch/agat/encoder.cc",
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
        "./arch/rolandd20/decoder.cc",
        "./arch/smaky6/decoder.cc",
        "./arch/tids990/decoder.cc",
        "./arch/tids990/encoder.cc",
        "./arch/victor9k/decoder.cc",
        "./arch/victor9k/encoder.cc",
        "./arch/zilogmcz/decoder.cc",
    ],
    hdrs={
        "arch/ibm/ibm.h": "./arch/ibm/ibm.h",
        "arch/apple2/data_gcr.h": "./arch/apple2/data_gcr.h",
        "arch/apple2/apple2.h": "./arch/apple2/apple2.h",
        "arch/smaky6/smaky6.h": "./arch/smaky6/smaky6.h",
        "arch/tids990/tids990.h": "./arch/tids990/tids990.h",
        "arch/zilogmcz/zilogmcz.h": "./arch/zilogmcz/zilogmcz.h",
        "arch/amiga/amiga.h": "./arch/amiga/amiga.h",
        "arch/f85/data_gcr.h": "./arch/f85/data_gcr.h",
        "arch/f85/f85.h": "./arch/f85/f85.h",
        "arch/mx/mx.h": "./arch/mx/mx.h",
        "arch/aeslanier/aeslanier.h": "./arch/aeslanier/aeslanier.h",
        "arch/northstar/northstar.h": "./arch/northstar/northstar.h",
        "arch/brother/data_gcr.h": "./arch/brother/data_gcr.h",
        "arch/brother/brother.h": "./arch/brother/brother.h",
        "arch/brother/header_gcr.h": "./arch/brother/header_gcr.h",
        "arch/macintosh/data_gcr.h": "./arch/macintosh/data_gcr.h",
        "arch/macintosh/macintosh.h": "./arch/macintosh/macintosh.h",
        "arch/agat/agat.h": "./arch/agat/agat.h",
        "arch/fb100/fb100.h": "./arch/fb100/fb100.h",
        "arch/victor9k/data_gcr.h": "./arch/victor9k/data_gcr.h",
        "arch/victor9k/victor9k.h": "./arch/victor9k/victor9k.h",
        "arch/rolandd20/rolandd20.h": "./arch/rolandd20/rolandd20.h",
        "arch/micropolis/micropolis.h": "./arch/micropolis/micropolis.h",
        "arch/c64/data_gcr.h": "./arch/c64/data_gcr.h",
        "arch/c64/c64.h": "./arch/c64/c64.h",
        "lib/a2r.h": "./lib/a2r.h",
        "lib/bitmap.h": "./lib/bitmap.h",
        "lib/bytes.h": "./lib/bytes.h",
        "lib/config.h": "./lib/config.h",
        "lib/crc.h": "./lib/crc.h",
        "lib/csvreader.h": "./lib/csvreader.h",
        "lib/decoders/decoders.h": "./lib/decoders/decoders.h",
        "lib/decoders/fluxdecoder.h": "./lib/decoders/fluxdecoder.h",
        "lib/decoders/fluxmapreader.h": "./lib/decoders/fluxmapreader.h",
        "lib/decoders/rawbits.h": "./lib/decoders/rawbits.h",
        "lib/encoders/encoders.h": "./lib/encoders/encoders.h",
        "lib/scp.h": "./lib/scp.h",
        "lib/fl2.h": "./lib/fl2.h",
        "lib/flags.h": "./lib/flags.h",
        "lib/flux.h": "./lib/flux.h",
        "lib/fluxmap.h": "./lib/fluxmap.h",
        "lib/fluxsink/fluxsink.h": "./lib/fluxsink/fluxsink.h",
        "lib/fluxsource/fluxsource.h": "lib/fluxsource/fluxsource.h",
        "lib/fluxsource/flx.h": "lib/fluxsource/flx.h",
        "lib/fluxsource/kryoflux.h": "lib/fluxsource/kryoflux.h",
        "lib/globals.h": "./lib/globals.h",
        "lib/image.h": "./lib/image.h",
        "lib/imagereader/imagereader.h": "./lib/imagereader/imagereader.h",
        "lib/imagewriter/imagewriter.h": "./lib/imagewriter/imagewriter.h",
        "lib/layout.h": "./lib/layout.h",
        "lib/ldbs.h": "./lib/ldbs.h",
        "lib/logger.h": "./lib/logger.h",
        "lib/proto.h": "./lib/proto.h",
        "lib/readerwriter.h": "./lib/readerwriter.h",
        "lib/sector.h": "./lib/sector.h",
        "lib/usb/greaseweazle.h": "./lib/usb/greaseweazle.h",
        "lib/usb/usb.h": "./lib/usb/usb.h",
        "lib/usb/usbfinder.h": "./lib/usb/usbfinder.h",
        "lib/utils.h": "./lib/utils.h",
        "lib/vfs/applesingle.h": "./lib/vfs/applesingle.h",
        "lib/vfs/sectorinterface.h": "./lib/vfs/sectorinterface.h",
        "lib/vfs/vfs.h": "./lib/vfs/vfs.h",
    },
    deps=[
        "+fl2_proto_lib",
        "+protocol",
        "lib+config_proto_lib",
        "dep/adflib",
        "dep/agg",
        "dep/fatfs",
        "dep/hfsutils",
        "dep/libusbp",
        "dep/stb",
    ],
)

export(
    name="all",
    items={
        "fluxengine": "src+fluxengine",
        "fluxengine-gui": "src/gui",
        "brother120tool": "tools+brother120tool",
        "brother240tool": "tools+brother240tool",
        "upgrade-flux-file": "tools+upgrade-flux-file",
    },
    deps=["tests"],
)