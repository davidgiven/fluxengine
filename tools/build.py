from build.c import cxxprogram
import config

emu = []
if config.windows:
    emu = ["dep/emu"]

cxxprogram(
    name="brother120tool",
    srcs=["./brother120tool.cc"],
    deps=[
        "+fmt_lib",
        "+protobuf_lib",
        "+z_lib",
        "lib/config",
        "lib/core",
        "lib/data",
        "lib/fluxsource+proto_lib",
        "lib/algorithms",
        "src/formats",
    ]
    + emu,
)

cxxprogram(
    name="brother240tool",
    srcs=["./brother240tool.cc"],
    deps=[
        "+fmt_lib",
        "+protobuf_lib",
        "+z_lib",
        "lib/config",
        "lib/core",
        "lib/data",
        "lib/fluxsource+proto_lib",
        "lib/algorithms",
        "src/formats",
    ]
    + emu,
)

cxxprogram(
    name="upgrade-flux-file",
    srcs=["./upgrade-flux-file.cc"],
    deps=[
        "+fmt_lib",
        "+protobuf_lib",
        "+protocol",
        "+sqlite3_lib",
        "+z_lib",
        "dep/libusbp",
        "lib/config+proto_lib",
        "lib/core",
        "lib/data",
        "lib/external+fl2_proto_lib",
        "lib/fluxsource+proto_lib",
        "lib/algorithms",
        "src/formats",
    ],
)
