from build.c import cxxlibrary
from build.protobuf import proto, protocc

proto(
    name="drive_proto",
    srcs=["./drive.proto"],
    deps=["lib+common_proto", "lib/external+fl2_proto", "lib+layout_proto"],
)

protocc(
    name="drive_proto_lib",
    srcs=[".+drive_proto"],
    deps=[
        "lib+common_proto_lib",
        "lib/external+fl2_proto_lib",
        "lib+layout_proto_lib",
    ],
)
proto(
    name="proto",
    srcs=["./config.proto"],
    deps=[
        "lib+common_proto",
        "lib+layout_proto",
        ".+drive_proto",
        "lib/external+fl2_proto",
        "lib/fluxsource+proto",
        "lib/fluxsink+proto",
        "lib/vfs+proto",
        "lib/usb+proto",
        "lib/encoders+proto",
        "lib/decoders+proto",
        "lib/imagereader+proto",
        "lib/imagewriter+proto",
    ],
)

protocc(
    name="proto_lib",
    srcs=[".+proto", "arch+proto"],
    deps=[
        ".+drive_proto_lib",
        "lib/decoders+proto_lib",
        "lib/encoders+proto_lib",
        "lib/external+fl2_proto_lib",
        "lib/fluxsink+proto_lib",
        "lib/fluxsource+proto_lib",
        "lib/imagereader+proto_lib",
        "lib/imagewriter+proto_lib",
        "lib/usb+proto_lib",
        "lib/vfs+proto_lib",
    ],
)

cxxlibrary(
    name="config",
    srcs=[
        "./config.cc",
        "./proto.cc",
        "./flags.cc",
    ],
    hdrs={
        "lib/config/config.h": "./config.h",
        "lib/config/proto.h": "./proto.h",
        "lib/config/flags.h": "./flags.h",
    },
    deps=[
        "lib/core",
        ".+proto_lib",
        "+fmt_lib",
    ],
)
