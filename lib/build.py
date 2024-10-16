from build.protobuf import proto, protocc


proto(name="common_proto", srcs=["./common.proto"])
protocc(
    name="common_proto_lib", srcs=[".+common_proto"], deps=["+protobuf_lib"]
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
    name="drive_proto",
    srcs=["./drive.proto"],
    deps=[".+common_proto", "lib/external+fl2_proto", ".+layout_proto"],
)
protocc(
    name="drive_proto_lib",
    srcs=[".+drive_proto"],
    deps=[
        ".+common_proto_lib",
        "lib/external+fl2_proto_lib",
        ".+layout_proto_lib",
    ],
)
