from build.c import cxxlibrary
from build.protobuf import proto, protocc


proto(name="common_proto", srcs=["./common.proto"])

proto(
    name="config_proto",
    srcs=[
        "./config.proto",
        "./layout.proto",
        "./drive.proto",
        "./decoders/decoders.proto",
        "./encoders/encoders.proto",
        "./fluxsink/fluxsink.proto",
        "./fluxsource/fluxsource.proto",
        "./imagereader/imagereader.proto",
        "./imagewriter/imagewriter.proto",
        "./usb/usb.proto",
        "./vfs/vfs.proto",
    ],
    deps=[".+common_proto", "+fl2_proto"],
)

protocc(
    name="config_proto_lib",
    srcs=[".+common_proto", ".+config_proto", "arch+arch_proto", "+fl2_proto"]
)
