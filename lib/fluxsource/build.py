from build.protobuf import proto, protocc

proto(name="proto", srcs=["./fluxsource.proto"], deps=["lib+common_proto"])

protocc(
    name="proto_lib",
    srcs=[".+proto"],
    deps=["lib+common_proto", "lib+common_proto_lib"],
)
