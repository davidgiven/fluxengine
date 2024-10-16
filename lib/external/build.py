from build.c import cxxlibrary
from build.protobuf import proto, protocc

proto(name="fl2_proto", srcs=["./fl2.proto"])
protocc(name="fl2_proto_lib", srcs=[".+fl2_proto"], deps=["+protobuf_lib"])

cxxlibrary(
    name="external",
    srcs=[
        "./catweasel.cc",
        "./csvreader.cc",
        "./fl2.cc",
        "./flx.cc",
        "./greaseweazle.cc",
        "./kryoflux.cc",
        "./ldbs.cc",
    ],
    hdrs={
        "lib/external/a2r.h": "./a2r.h",
        "lib/external/applesauce.h": "./applesauce.h",
        "lib/external/catweasel.h": "./catweasel.h",
        "lib/external/csvreader.h": "./csvreader.h",
        "lib/external/fl2.h": "./fl2.h",
        "lib/external/flx.h": "./flx.h",
        "lib/external/greaseweazle.h": "./greaseweazle.h",
        "lib/external/kryoflux.h": "./kryoflux.h",
        "lib/external/ldbs.h": "./ldbs.h",
        "lib/external/scp.h": "./scp.h",
    },
    deps=["lib/core", ".+fl2_proto_lib", "lib/data"],
)
