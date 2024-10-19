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
