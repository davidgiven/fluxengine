from build.protobuf import proto, protocc


proto(
    name="proto",
    srcs=["./encoders.proto"],
    deps=["lib+common_proto", "arch+proto"],
)
protocc(
    name="proto_lib",
    srcs=[".+proto"],
    deps=["lib+common_proto_lib", "arch+proto_lib"],
)
