from build.ab import Rule, simplerule, Targets, TargetsMap
from build.c import cxxprogram

encoders = {}


@Rule
def protoencode_single(self, name, srcs: Targets, proto, symbol):
    if proto not in encoders:
        r = cxxprogram(
            name="protoencode_" + proto,
            srcs=["scripts/protoencode.cc"],
            cflags=["-DPROTO=" + proto],
            deps=[
                "lib/config+proto_lib",
                "lib/fluxsource+proto_lib",
                "lib/fluxsink+proto_lib",
                "tests+test_proto_lib",
                "+protobuf_lib",
                "+fmt_lib",
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
            # Materialise symbolic links (for Windows).
            "cp -L $[ins[0]] $[ins[0]].real",
            "mv $[ins[0]].real $[ins[0]]",
            "$[deps[0]] $[ins] $[outs] " + symbol
        ],
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
        "+fmt_lib",
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
        "+fmt_lib",
        "+protobuf_lib",
        "lib/algorithms",
        "lib/config+proto_lib",
        "lib/fluxsink+proto_lib",
        "lib/fluxsource+proto_lib",
        "src/formats",
    ],
)
