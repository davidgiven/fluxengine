from build.protobuf import proto, protocc
from build.c import cxxlibrary

proto(
    name="proto",
    srcs=["./decoders.proto"],
    deps=["lib/config+common_proto", "arch+proto", "lib/fluxsink+proto"],
)

protocc(
    name="proto_lib",
    srcs=[".+proto"],
    deps=[
        "lib/config+common_proto_lib",
        "arch+proto_lib",
        "lib/fluxsink+proto_lib",
    ],
)

cxxlibrary(
    name="decoders",
    srcs=["./decoders.cc", "./fluxdecoder.cc", "./fmmfm.cc"],
    hdrs={
        "lib/decoders/decoders.h": "./decoders.h",
        "lib/decoders/fluxdecoder.h": "./fluxdecoder.h",
        "lib/decoders/rawbits.h": "./rawbits.h",
    },
    deps=["lib/core", "lib/config", "lib/data", ".+proto_lib"],
)
