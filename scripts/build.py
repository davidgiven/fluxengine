from build.ab import Rule, normalrule, Targets, TargetsMap
from build.c import cxxprogram, HostToolchain

encoders = {}


@Rule
def protoencode_single(self, name, srcs: Targets, proto, symbol):
    if proto not in encoders:
        r = cxxprogram(
            name="protoencode_" + proto,
            srcs=["scripts/protoencode.cc"],
            cflags=["-DPROTO=" + proto],
            deps=[
                "lib+config_proto_lib",
                "tests+test_proto_lib",
                "+protobuf_lib",
                "+fmt_lib",
                "+lib",
            ],
        )
        encoders[proto] = r
    else:
        r = encoders[proto]
    r.materialise()

    normalrule(
        replaces=self,
        ins=srcs,
        outs=[f"{name}.cc"],
        deps=[r],
        commands=["{deps[0]} {ins} {outs} " + symbol],
        label="PROTOENCODE",
    )


@Rule
def protoencode(self, name, proto, srcs: TargetsMap, symbol):
    encoded = [
        protoencode_single(
            name=f"{k}_cc",
            srcs=[v],
            proto=proto,
            symbol=f"{symbol}_{k}_pb",
        )
        for k, v in srcs.items()
    ]

    normalrule(
        replaces=self,
        ins=encoded,
        outs=[name + ".cc"],
        commands=["cat {ins} > {outs}"],
        label="CONCAT",
    )


cxxprogram(
    name="mkdoc",
    srcs=["./mkdoc.cc"],
    deps=[
        "src/formats",
        "lib+config_proto_lib",
        "+lib",
        "+fmt_lib",
        "+protobuf_lib",
    ],
)

cxxprogram(
    name="mkdocindex",
    srcs=["./mkdocindex.cc"],
    deps=[
        "src/formats",
        "lib+config_proto_lib",
        "+lib",
        "+fmt_lib",
        "+protobuf_lib",
    ],
)
