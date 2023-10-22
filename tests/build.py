from build.ab import export
from build.c import cxxprogram
from build.protobuf import proto, protocc
from build.utils import test
from scripts.build import protoencode


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

tests = [
    "agg",
    "amiga",
    "applesingle",
    "bitaccumulator",
    "bytes",
    "compression",
    "configs",
    "cpmfs",
    "csvreader",
    "flags",
    "fluxmapreader",
    "fluxpattern",
    "flx",
    "fmmfm",
    "greaseweazle",
    "kryoflux",
    "layout",
    "ldbs",
    "options",
    "utils",
    "vfs",
]

export(
    name="tests",
    deps=[
        test(
            name="proto_test",
            command=cxxprogram(
                name="proto_test_exe",
                srcs=[
                    "./proto.cc",
                    protoencode(
                        name="testproto_cc",
                        srcs=["./testproto.textpb"],
                        proto="TestProto",
                        symbol="testproto_pb",
                    ),
                ],
                deps=[
                    "+fl2_proto_lib",
                    "+protocol",
                    ".+test_proto_lib",
                    "dep/adflib",
                    "dep/agg",
                    "dep/fatfs",
                    "dep/hfsutils",
                    "dep/libusbp",
                    "dep/snowhouse",
                    "dep/stb",
                    "+lib",
                    "lib+config_proto_lib",
                    "src/formats",
                ],
            ),
        ),
    ]
    + [
        test(
            name=f"{n}_test",
            command=cxxprogram(
                name=f"{n}_test_exe",
                srcs=[f"./{n}.cc"],
                deps=[
                    "+fl2_proto_lib",
                    "+protocol",
                    "dep/adflib",
                    "dep/agg",
                    "dep/fatfs",
                    "dep/hfsutils",
                    "dep/libusbp",
                    "dep/snowhouse",
                    "dep/stb",
                    "+lib",
                    "lib+config_proto_lib",
                    "src/formats",
                ],
            ),
        )
        for n in tests
    ],
)
