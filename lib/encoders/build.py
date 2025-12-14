from build.protobuf import proto, protocc
from build.c import cxxlibrary

proto(
    name="proto",
    srcs=["./encoders.proto"],
    deps=["lib/config+common_proto", "arch+proto"],
)
protocc(
    name="proto_lib",
    srcs=[".+proto"],
    deps=["lib/config+common_proto_lib", "arch+proto_lib"],
)

cxxlibrary(
    name="encoders",
    srcs=["./encoders.cc"],
    hdrs={"lib/encoders/encoders.h": "./encoders.h"},
    deps=["lib/core", "lib/data", "lib/config", ".+proto_lib"],
)
