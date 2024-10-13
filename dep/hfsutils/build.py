from build.c import clibrary

clibrary(
    name="hfsutils",
    srcs=[
        "./libhfs/block.c",
        "./libhfs/btree.c",
        "./libhfs/data.c",
        "./libhfs/file.c",
        "./libhfs/hfs.c",
        "./libhfs/low.c",
        "./libhfs/medium.c",
        "./libhfs/memcmp.c",
        "./libhfs/node.c",
        "./libhfs/record.c",
        "./libhfs/version.c",
        "./libhfs/volume.c",
    ],
    hdrs={
        "apple.h": "./libhfs/apple.h",
        "hfs.h": "./libhfs/hfs.h",
        "libhfs.h": "./libhfs/libhfs.h",
        "os.h": "./libhfs/os.h",
    },
    cflags=["-Wno-pointer-sign"],
)
