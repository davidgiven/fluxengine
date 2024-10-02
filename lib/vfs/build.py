from build.c import cxxlibrary

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
    ],
)
