from build.c import cxxprogram

cxxprogram(
    name="fluxengine",
    srcs=[
        "./fluxengine.cc",
        "./fe-analysedriveresponse.cc",
        "./fe-analyselayout.cc",
        "./fe-format.cc",
        "./fe-getdiskinfo.cc",
        "./fe-getfile.cc",
        "./fe-getfileinfo.cc",
        "./fe-inspect.cc",
        "./fe-ls.cc",
        "./fe-merge.cc",
        "./fe-mkdir.cc",
        "./fe-mv.cc",
        "./fe-putfile.cc",
        "./fe-rawread.cc",
        "./fe-rawwrite.cc",
        "./fe-read.cc",
        "./fe-rm.cc",
        "./fe-rpm.cc",
        "./fe-seek.cc",
        "./fe-testbandwidth.cc",
        "./fe-testdevices.cc",
        "./fe-testvoltages.cc",
        "./fe-write.cc",
        "./fileutils.cc",
    ],
    cflags=["-I."],
    deps=[
        "+fl2_proto_lib",
        "+fmt_lib",
        "+protobuf_lib",
        "+protocol",
        "+z_lib",
        "dep/adflib",
        "dep/agg",
        "dep/fatfs",
        "dep/hfsutils",
        "dep/libusbp",
        "dep/stb",
        "+lib",
        "lib+config_proto_lib",
        "src/formats",
    ],
)
