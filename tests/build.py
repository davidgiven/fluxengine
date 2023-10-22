from build.c import clibrary
from build.protobuf import proto, protocc


proto(
    name="test_proto",
    srcs=[
        "./testproto.proto",
    ],
)

protocc(
    name="test_proto_lib",
    srcs=[".+test_proto"],
    deps=["lib+config_proto", "arch+arch_proto"],
)
