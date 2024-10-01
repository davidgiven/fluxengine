from build.c import cxxlibrary

cxxlibrary(
    name="core",
    srcs=[
        "./bitmap.cc",
        "./bytes.cc",
        "./crc.cc",
        "./csvreader.cc",
        "./hexdump.cc",
        "./utils.cc",
    ],
    hdrs={
        "lib/core/bitmap.h": "./bitmap.h",
        "lib/core/bytes.h": "./bytes.h",
        "lib/core/crc.h": "./crc.h",
        "lib/core/csvreader.h": "./csvreader.h",
        "lib/core/globals.h": "./globals.h",
        "lib/core/utils.h": "./utils.h",
    },
    deps=[
        "dep/agg",
        "dep/stb",
    ],
)
