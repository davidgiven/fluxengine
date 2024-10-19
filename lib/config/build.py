from build.c import cxxlibrary
from build.protobuf import proto, protocc

proto(name="common_proto", srcs=["./common.proto"])
protocc(
    name="common_proto_lib", srcs=[".+common_proto"], deps=["+protobuf_lib"]
)

proto(
    name="drive_proto",
    srcs=["./drive.proto"],
    deps=[".+common_proto", "lib/external+fl2_proto", ".+layout_proto"],
)
protocc(
    name="drive_proto_lib",
    srcs=[".+drive_proto"],
    deps=[
        ".+layout_proto_lib",
        ".+common_proto_lib",
        "lib/external+fl2_proto_lib",
    ],
)

proto(
    name="layout_proto",
    srcs=["./layout.proto"],
    deps=[".+common_proto", "lib/external+fl2_proto"],
)
protocc(
    name="layout_proto_lib",
    srcs=[".+layout_proto"],
    deps=[".+common_proto_lib", "lib/external+fl2_proto_lib"],
)

proto(
    name="proto",
    srcs=["./config.proto"],
    deps=[
        ".+drive_proto",
        ".+layout_proto",
        ".+common_proto",
        "lib/decoders+proto",
        "lib/encoders+proto",
        "lib/external+fl2_proto",
        "lib/fluxsink+proto",
        "lib/fluxsource+proto",
        "lib/imagereader+proto",
        "lib/imagewriter+proto",
        "lib/usb+proto",
        "lib/vfs+proto",
    ],
)

protocc(
    name="proto_lib",
    srcs=[".+proto", "arch+proto"],
    deps=[
        ".+drive_proto_lib",
        ".+common_proto_lib",
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
