from build.protobuf import proto, protocc

proto(
    name="proto",
    srcs=["./decoders.proto"],
    deps=["lib+common_proto", "arch+proto", "lib/fluxsink+proto"],
)
protocc(
    name="proto_lib",
    srcs=[".+proto"],
    deps=["lib+common_proto_lib", "arch+proto_lib", "lib/fluxsink+proto_lib"],
)
