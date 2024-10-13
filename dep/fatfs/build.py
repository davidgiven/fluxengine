from build.c import clibrary

clibrary(
    name="fatfs",
    srcs=[
        "./source/ff.c",
        "./source/ffsystem.c",
        "./source/ffunicode.c",
        "./source/ff.h",
        "./source/ffconf.h",
        "./source/diskio.h",
    ],
    hdrs={
        "ff.h": "./source/ff.h",
        "ffconf.h": "./source/ffconf.h",
        "diskio.h": "./source/diskio.h",
    },
    cflags=["-Wno-pointer-sign"],
)
