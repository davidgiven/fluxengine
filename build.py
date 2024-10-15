from build.ab import export
from build.c import clibrary, cxxlibrary
from build.protobuf import proto, protocc
from build.pkg import package, hostpackage
from build.utils import test
from glob import glob
import config
import re

package(name="protobuf_lib", package="protobuf")
package(name="z_lib", package="zlib")
package(name="fmt_lib", package="fmt", fallback="dep/fmt")
package(name="sqlite3_lib", package="sqlite3")

hostpackage(name="protobuf_host_lib", package="protobuf")
hostpackage(name="z_host_lib", package="zlib")
hostpackage(name="fmt_host_lib", package="fmt", fallback="dep/fmt")
hostpackage(name="sqlite3_host_lib", package="sqlite3")

clibrary(name="protocol", hdrs={"protocol.h": "./protocol.h"})

cxxlibrary(
    name="lib",
    srcs=[
        "./lib/decoders/decoders.cc",
        "./lib/decoders/fluxdecoder.cc",
        "./lib/decoders/fmmfm.cc",
        "./lib/encoders/encoders.cc",
        "./lib/fluxsource/a2rfluxsource.cc",
        "./lib/fluxsource/cwffluxsource.cc",
        "./lib/fluxsource/dmkfluxsource.cc",
        "./lib/fluxsource/erasefluxsource.cc",
        "./lib/fluxsource/fl2fluxsource.cc",
        "./lib/fluxsource/fluxsource.cc",
        "./lib/fluxsource/flxfluxsource.cc",
        "./lib/fluxsource/hardwarefluxsource.cc",
        "./lib/fluxsource/kryofluxfluxsource.cc",
        "./lib/fluxsource/memoryfluxsource.cc",
        "./lib/fluxsource/scpfluxsource.cc",
        "./lib/fluxsource/testpatternfluxsource.cc",
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
        "./lib/readerwriter.cc",
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
        "./arch/tartu/decoder.cc",
        "./arch/tartu/encoder.cc",
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
        "arch/tartu/tartu.h": "./arch/tartu/tartu.h",
        "lib/decoders/decoders.h": "./lib/decoders/decoders.h",
        "lib/decoders/fluxdecoder.h": "./lib/decoders/fluxdecoder.h",
        "lib/decoders/rawbits.h": "./lib/decoders/rawbits.h",
        "lib/encoders/encoders.h": "./lib/encoders/encoders.h",
        "lib/fluxsource/fluxsource.h": "lib/fluxsource/fluxsource.h",
        "lib/imagereader/imagereader.h": "./lib/imagereader/imagereader.h",
        "lib/imagewriter/imagewriter.h": "./lib/imagewriter/imagewriter.h",
        "lib/readerwriter.h": "./lib/readerwriter.h",
    },
    deps=[
        "+fmt_lib",
        "+protocol",
        "dep/adflib",
        "dep/fatfs",
        "dep/hfsutils",
        "dep/libusbp",
        "dep/stb",
        "src/formats",
        "lib/core",
        "lib/config",
        "lib/data",
        "lib/external",
        "lib/fluxsink",
        "lib/fluxsource+proto_lib",
    ],
)

corpustests = []
if not glob("../fluxengine-testdata/data"):
    print("fluxengine-testdata not found; skipping corpus tests")
else:
    corpus = [
        ("acorndfs", "", "--200"),
        ("agat", "", ""),
        ("amiga", "", ""),
        ("apple2", "", "--140 40track_drive"),
        ("atarist", "", "--360"),
        ("atarist", "", "--370"),
        ("atarist", "", "--400"),
        ("atarist", "", "--410"),
        ("atarist", "", "--720"),
        ("atarist", "", "--740"),
        ("atarist", "", "--800"),
        ("atarist", "", "--820"),
        ("bk", "", ""),
        ("brother", "", "--120 40track_drive"),
        ("brother", "", "--240"),
        (
            "commodore",
            "scripts/commodore1541_test.textpb",
            "--171 40track_drive",
        ),
        (
            "commodore",
            "scripts/commodore1541_test.textpb",
            "--192 40track_drive",
        ),
        ("commodore", "", "--800"),
        ("commodore", "", "--1620"),
        ("hplif", "", "--264"),
        ("hplif", "", "--608"),
        ("hplif", "", "--616"),
        ("hplif", "", "--770"),
        ("ibm", "", "--1200"),
        ("ibm", "", "--1232"),
        ("ibm", "", "--1440"),
        ("ibm", "", "--1680"),
        ("ibm", "", "--180 40track_drive"),
        ("ibm", "", "--160 40track_drive"),
        ("ibm", "", "--320 40track_drive"),
        ("ibm", "", "--360 40track_drive"),
        ("ibm", "", "--720_96"),
        ("ibm", "", "--720_135"),
        ("mac", "scripts/mac400_test.textpb", "--400"),
        ("mac", "scripts/mac800_test.textpb", "--800"),
        ("n88basic", "", ""),
        ("rx50", "", ""),
        ("tartu", "", "--390 40track_drive"),
        ("tartu", "", "--780"),
        ("tids990", "", ""),
        ("victor9k", "", "--612"),
        ("victor9k", "", "--1224"),
    ]

    for c in corpus:
        name = re.sub(r"[^a-zA-Z0-9]", "_", "".join(c), 0)
        corpustests += [
            test(
                name=f"corpustest_{name}_{format}",
                ins=["src+fluxengine"],
                deps=["scripts/encodedecodetest.sh"],
                commands=[
                    "{deps[0]} "
                    + c[0]
                    + " "
                    + format
                    + " {ins[0]} '"
                    + c[1]
                    + "' '"
                    + c[2]
                    + "' $(dir {outs[0]}) > /dev/null"
                ],
                label="CORPUSTEST",
            )
            for format in ["scp", "flux"]
        ]

export(
    name="all",
    items={
        "fluxengine$(EXT)": "src+fluxengine",
        "fluxengine-gui$(EXT)": "src/gui",
        "brother120tool$(EXT)": "tools+brother120tool",
        "brother240tool$(EXT)": "tools+brother240tool",
        "upgrade-flux-file$(EXT)": "tools+upgrade-flux-file",
    }
    | ({"FluxEngine.pkg": "src/gui+fluxengine_pkg"} if config.osx else {}),
    deps=["tests", "src/formats+docs", "scripts+mkdocindex"] + corpustests,
)
