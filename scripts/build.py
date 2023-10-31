from build.ab import Rule, normalrule, Targets
from build.c import cxxprogram

encoders = {}


@Rule
def protoencode(self, name, srcs: Targets, proto, symbol):
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


cxxprogram(
    name="mkdoc",
    srcs=["./mkdoc.cc"],
    deps=["src/formats", "lib+config_proto_lib", "+lib"],
)

cxxprogram(
    name="mkdocindex",
    srcs=["./mkdocindex.cc"],
    deps=["src/formats", "lib+config_proto_lib", "+lib"],
)
