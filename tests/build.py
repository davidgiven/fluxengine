from build.ab import export
from build.c import cxxprogram
from build.protobuf import proto, protocc
from build.utils import test
from scripts.build import protoencode_single


proto(
    name="test_proto",
    srcs=["./testproto.proto"],
    deps=["lib/config+common_proto"],
)

protocc(
    name="test_proto_lib", srcs=[".+test_proto"], deps=["lib/config+proto_lib"]
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
                    protoencode_single(
                        name="testproto_cc",
                        srcs=["./testproto.textpb"],
                        proto="TestProto",
                        symbol="testproto_pb",
                    ),
                ],
                deps=[
                    "lib/external+fl2_proto_lib",
                    "+fmt_lib",
                    "+protobuf_lib",
                    "+protocol",
                    "+z_lib",
                    ".+test_proto_lib",
                    "dep/adflib",
                    "dep/agg",
                    "dep/fatfs",
                    "dep/hfsutils",
                    "dep/libusbp",
                    "dep/snowhouse",
                    "dep/stb",
                    "lib/config",
                    "lib/core",
                    "lib/data",
                    "lib/fluxsource+proto_lib",
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
                    "lib/external+fl2_proto_lib",
                    "+fmt_lib",
                    "+protobuf_lib",
                    "+protocol",
                    "+z_lib",
                    "arch+proto_lib",
                    "dep/adflib",
                    "dep/agg",
                    "dep/fatfs",
                    "dep/hfsutils",
                    "dep/libusbp",
                    "dep/snowhouse",
                    "dep/stb",
                    "lib/algorithms",
                    "lib/config",
                    "lib/core",
                    "lib/data",
                    "lib/fluxsource+proto_lib",
                    "src/formats",
                ]
                + ([".+test_proto_lib"] if n == "options" else [])
                + (["lib/vfs"] if n in {"cpmfs", "applesingle", "vfs"} else [])
                + (["arch"] if n in {"amiga"} else []),
            ),
        )
        for n in tests
    ],
)
