from build.c import cxxprogram
import config

emu = []
if config.windows:
    emu = ["dep/emu"]

cxxprogram(
    name="brother120tool",
    srcs=["./brother120tool.cc"],
    deps=[
        "+lib",
        "lib/core",
        "lib+config_proto_lib",
        "lib/fluxsource+proto_lib",
        "+fmt_lib",
        "+z_lib",
    ]
    + emu,
)

cxxprogram(
    name="brother240tool",
    srcs=["./brother240tool.cc"],
    deps=[
        "+lib",
        "lib/core",
        "lib+config_proto_lib",
        "lib/fluxsource+proto_lib",
        "+fmt_lib",
        "+z_lib",
    ]
    + emu,
)

cxxprogram(
    name="upgrade-flux-file",
    srcs=["./upgrade-flux-file.cc"],
    deps=[
        "+fl2_proto_lib",
        "+fmt_lib",
        "+lib",
        "+protobuf_lib",
        "+protocol",
        "+sqlite3_lib",
        "+z_lib",
        "dep/libusbp",
        "lib+config_proto_lib",
        "lib/fluxsource+proto_lib",
        "src/formats",
        "lib/core",
    ],
)
