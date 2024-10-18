from build.protobuf import proto, protocc
from build.c import cxxlibrary

proto(name="proto", srcs=["./fluxsource.proto"], deps=["lib+common_proto"])

protocc(
    name="proto_lib",
    srcs=[".+proto"],
    deps=["lib+common_proto", "lib+common_proto_lib"],
)

cxxlibrary(
    name="fluxsource",
    srcs=[
        "./a2rfluxsource.cc",
        "./cwffluxsource.cc",
        "./dmkfluxsource.cc",
        "./erasefluxsource.cc",
        "./fl2fluxsource.cc",
        "./fluxsource.cc",
        "./flxfluxsource.cc",
        "./hardwarefluxsource.cc",
        "./kryofluxfluxsource.cc",
        "./memoryfluxsource.cc",
        "./scpfluxsource.cc",
        "./testpatternfluxsource.cc",
    ],
    hdrs={"lib/fluxsource/fluxsource.h": "./fluxsource.h"},
    deps=["lib/core", "lib/data", "lib/external", "lib/usb", ".+proto_lib"],
)
