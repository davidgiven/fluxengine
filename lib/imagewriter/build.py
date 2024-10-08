from build.protobuf import proto, protocc

proto(
    name="proto",
    srcs=["./imagewriter.proto"],
    deps=["lib+common_proto", "lib/imagereader+proto"],
)
protocc(
    name="proto_lib",
    srcs=[".+proto"],
    deps=["lib+common_proto_lib", "lib/imagereader+proto_lib"],
)
