from build.c import cxxlibrary
from build.protobuf import proto, protocc
from os.path import *
from glob import glob
import sys

archs = [f for f in glob("*", root_dir="arch") if isfile(f"arch/{f}/{f}.proto")]

ps = []
pls = []
cls = []
for a in archs:
    ps += [
        proto(
            name=f"proto_{a}",
            srcs=[f"arch/{a}/{a}.proto"],
            deps=["lib/config+common_proto"],
        )
    ]

    pls += [
        protocc(
            name=f"proto_lib_{a}",
            srcs=[f".+proto_{a}"],
            deps=["lib/config+common_proto_lib"],
        )
    ]

    cls += [
        cxxlibrary(
            name=f"arch_{a}",
            srcs=glob(f"arch/{a}/*.cc") + glob(f"arch/{a}/*.h"),
            hdrs={f"arch/{a}/{a}.h": f"arch/{a}/{a}.h"},
            deps=[
                "lib/core",
                "lib/data",
                "lib/config",
                "lib/encoders",
                "lib/decoders",
            ],
        )
    ]

proto(
    name="proto",
    deps=ps + ["lib/config+common_proto"],
)

cxxlibrary(name="proto_lib", deps=pls)

cxxlibrary(
    name="arch",
    srcs=[
        "./arch.cc",
    ],
    hdrs={
        "arch/arch.h": "./arch.h",
    },
    deps=cls
    + ["lib/core", "lib/data", "lib/config", "lib/encoders", "lib/decoders"],
)
