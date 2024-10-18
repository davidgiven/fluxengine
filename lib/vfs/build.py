from build.c import cxxlibrary
from build.protobuf import proto, protocc

proto(
    name="proto",
    srcs=["./vfs.proto"],
    deps=["lib+common_proto", "lib+layout_proto", "lib/external+fl2_proto"],
)

protocc(
    name="proto_lib",
    srcs=[".+proto"],
    deps=[
        "lib+common_proto_lib",
        "lib+layout_proto_lib",
        "lib/external+fl2_proto_lib",
    ],
)

cxxlibrary(
    name="vfs",
    srcs=[
        "./acorndfs.cc",
        "./amigaffs.cc",
        "./appledos.cc",
        "./applesingle.cc",
        "./brother120fs.cc",
        "./cbmfs.cc",
        "./cpmfs.cc",
        "./fatfs.cc",
        "./fluxsectorinterface.cc",
        "./imagesectorinterface.cc",
        "./lif.cc",
        "./machfs.cc",
        "./microdos.cc",
        "./philefs.cc",
        "./prodos.cc",
        "./roland.cc",
        "./smaky6fs.cc",
        "./vfs.cc",
        "./zdos.cc",
    ],
    hdrs={
        "lib/vfs/applesingle.h": "./applesingle.h",
        "lib/vfs/sectorinterface.h": "./sectorinterface.h",
        "lib/vfs/vfs.h": "./vfs.h",
    },
    deps=[
        "+lib",
        "+fmt_lib",
        "arch",
        ".+proto_lib",
    ],
)
