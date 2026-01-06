from build.ab import Rule, simplerule, Targets, TargetsMap
from build.c import cxxprogram

encoders = {}


@Rule
def protoencode_single(self, name, srcs: Targets, proto, include, symbol):
    if proto not in encoders:
        r = cxxprogram(
            name="protoencode_" + proto,
            srcs=["scripts/protoencode.cc"],
            cflags=["-DPROTO=" + proto, "-DINCLUDE="+include],
            deps=[
                "lib/core",
                "lib/config+proto_lib",
                "lib/fluxsource+proto_lib",
                "lib/fluxsink+proto_lib",
                "tests+test_proto_lib",
                "+protobuf_lib",
                "dep+fmt_lib",
            ],
        )
        encoders[proto] = r
    else:
        r = encoders[proto]
    r.materialise()

    simplerule(
        replaces=self,
        ins=srcs,
        outs=[f"={name}.cc"],
        deps=[r],
        commands=[
            "$[deps[0]] $[ins] $[outs] " + symbol
        ],
        label="PROTOENCODE",
    )


@Rule
def protoencode(self, name, proto, include,srcs: TargetsMap, symbol):
    encoded = [
        protoencode_single(
            name=f"{k}_cc",
            srcs=[v],
            proto=proto,
            include=include,
            symbol=f"{symbol}_{k}_pb",
        )
        for k, v in srcs.items()
    ]

    simplerule(
        replaces=self,
        ins=encoded,
        outs=[f"={name}.cc"],
        commands=["cat $[ins] > $[outs]"],
        label="CONCAT",
    )


cxxprogram(
    name="mkdoc",
    srcs=["./mkdoc.cc"],
    deps=[
        "dep+fmt_lib",
        "+protobuf_lib",
        "lib/algorithms",
        "lib/config+proto_lib",
        "lib/fluxsink+proto_lib",
        "lib/fluxsource+proto_lib",
        "src/formats",
    ],
)

cxxprogram(
    name="mkdocindex",
    srcs=["./mkdocindex.cc"],
    deps=[
        "dep+fmt_lib",
        "+protobuf_lib",
        "lib/algorithms",
        "lib/config+proto_lib",
        "lib/fluxsink+proto_lib",
        "lib/fluxsource+proto_lib",
        "src/formats",
    ],
)
