from build.protobuf import proto, protocc
from build.c import cxxlibrary

proto(
    name="proto",
    srcs=["./imagewriter.proto"],
    deps=["lib/config+common_proto", "lib/imagereader+proto"],
)
protocc(
    name="proto_lib",
    srcs=[".+proto"],
    deps=["lib/config+common_proto_lib", "lib/imagereader+proto_lib"],
)

cxxlibrary(
    name="imagewriter",
    srcs=[
        "./d64imagewriter.cc",
        "./d88imagewriter.cc",
        "./diskcopyimagewriter.cc",
        "./imagewriter.cc",
        "./imdimagewriter.cc",
        "./imgimagewriter.cc",
        "./ldbsimagewriter.cc",
        "./nsiimagewriter.cc",
        "./rawimagewriter.cc",
    ],
    hdrs={
        "lib/imagewriter/imagewriter.h": "./imagewriter.h",
    },
    deps=["lib/core", "lib/data", "lib/external", ".+proto_lib"],
)
