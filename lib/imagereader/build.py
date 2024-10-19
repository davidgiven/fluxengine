from build.protobuf import proto, protocc
from build.c import cxxlibrary

proto(
    name="proto",
    srcs=["./imagereader.proto"],
    deps=["lib/config+common_proto"],
)
protocc(
    name="proto_lib",
    srcs=[".+proto"],
    deps=["lib/config+common_proto_lib"],
)

cxxlibrary(
    name="imagereader",
    srcs=[
        "./d64imagereader.cc",
        "./d88imagereader.cc",
        "./dimimagereader.cc",
        "./diskcopyimagereader.cc",
        "./fdiimagereader.cc",
        "./imagereader.cc",
        "./imdimagereader.cc",
        "./imgimagereader.cc",
        "./jv3imagereader.cc",
        "./nfdimagereader.cc",
        "./nsiimagereader.cc",
        "./td0imagereader.cc",
    ],
    hdrs={"lib/imagereader/imagereader.h": "./imagereader.h"},
    deps=["lib/core", "lib/config", "lib/data", ".+proto_lib"],
)
