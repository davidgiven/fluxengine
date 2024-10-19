from build.protobuf import proto, protocc
from build.c import cxxlibrary

proto(name="proto", srcs=["./fluxsink.proto"], deps=["lib/config+common_proto"])
protocc(
    name="proto_lib", srcs=[".+proto"], deps=["lib/config+common_proto_lib"]
)

cxxlibrary(
    name="fluxsink",
    srcs=[
        "./a2rfluxsink.cc",
        "./aufluxsink.cc",
        "./fl2fluxsink.cc",
        "./fluxsink.cc",
        "./hardwarefluxsink.cc",
        "./scpfluxsink.cc",
        "./vcdfluxsink.cc",
    ],
    hdrs={"lib/fluxsink/fluxsink.h": "./fluxsink.h"},
    deps=["lib/core", "lib/config", "lib/data", "lib/external", "lib/usb"],
)
