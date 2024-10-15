from build.c import cxxlibrary
from build.protobuf import proto, protocc

proto(name="fl2_proto", srcs=["./fl2.proto"])
protocc(name="fl2_proto_lib", srcs=[".+fl2_proto"], deps=["+protobuf_lib"])

cxxlibrary(
    name="external",
    srcs=[
        "./ldbs.cc",
        "./fl2.cc",
        "./kryoflux.cc",
        "./catweasel.cc",
        "./csvreader.cc",
    ],
    hdrs={
        "lib/external/a2r.h": "./a2r.h",
        "lib/external/catweasel.h": "./catweasel.h",
        "lib/external/csvreader.h": "./csvreader.h",
        "lib/external/fl2.h": "./fl2.h",
        "lib/external/kryoflux.h": "./kryoflux.h",
        "lib/external/ldbs.h": "./ldbs.h",
        "lib/external/scp.h": "./scp.h",
    },
    deps=["lib/core", ".+fl2_proto_lib", "lib/data"],
)
