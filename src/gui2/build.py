from build.ab import emit, simplerule
from build.c import cxxprogram
from glob import glob
import config

cxxprogram(
    name="gui2",
    srcs=glob("src/gui2/*.cc") + glob("src/gui2/*.h"),
    cflags=[],
    ldflags=[],
    deps=[
        "lib/external+fl2_proto_lib",
        "+protocol",
        "dep/adflib",
        "dep/fatfs",
        "dep/hfsutils",
        "dep/libusbp",
        "dep/imgui",
        "extras+icons",
        "lib/core",
        "lib/data",
        "lib/vfs",
        "lib/config",
        "arch",
        "src/formats",
        "src/gui/drivetypes",
        "+z_lib",
        "+fmt_lib",
        "+protobuf_lib",
    ],
)

