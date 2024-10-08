from build.protobuf import proto, protocc


proto(name="common_proto", srcs=["./common.proto"])
protocc(name="common_proto_lib", srcs=[".+common_proto"])

proto(
    name="layout_proto",
    srcs=["./layout.proto"],
    deps=[".+common_proto", "+fl2_proto"],
)
protocc(
    name="layout_proto_lib",
    srcs=[".+layout_proto"],
    deps=[".+common_proto_lib", "+fl2_proto_lib"],
)

proto(
    name="drive_proto",
    srcs=["./drive.proto"],
    deps=[".+common_proto", "+fl2_proto", ".+layout_proto"],
)
protocc(
    name="drive_proto_lib",
    srcs=[".+drive_proto"],
    deps=[".+common_proto_lib", "+fl2_proto_lib", ".+layout_proto_lib"],
)

proto(
    name="config_proto",
    srcs=[
        "./config.proto",
        "./imagewriter/imagewriter.proto",
    ],
    deps=[
        ".+common_proto",
        ".+layout_proto",
        ".+drive_proto",
        "+fl2_proto",
        "lib/fluxsource+proto",
        "lib/fluxsink+proto",
        "lib/vfs+proto",
        "lib/usb+proto",
        "lib/encoders+proto",
        "lib/decoders+proto",
        "lib/imagereader+proto",
    ],
)

protocc(
    name="config_proto_lib",
    srcs=[".+common_proto", ".+config_proto", "arch+proto", "+fl2_proto"],
    deps=[
        "lib/fluxsource+proto_lib",
        "lib/fluxsink+proto_lib",
        "lib/vfs+proto_lib",
        "lib/usb+proto_lib",
        "lib/encoders+proto_lib",
        "lib/decoders+proto_lib",
        "lib+drive_proto_lib",
    ],
)
