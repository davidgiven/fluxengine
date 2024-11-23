from build.c import cxxlibrary

cxxlibrary(
    name="core",
    srcs=[
        "./bitmap.cc",
        "./bytes.cc",
        "./crc.cc",
        "./hexdump.cc",
        "./utils.cc",
        "./logger.cc",
        "./logrenderer.cc",
    ],
    hdrs={
        "lib/core/bitmap.h": "./bitmap.h",
        "lib/core/bytes.h": "./bytes.h",
        "lib/core/crc.h": "./crc.h",
        "lib/core/globals.h": "./globals.h",
        "lib/core/utils.h": "./utils.h",
        "lib/core/logger.h": "./logger.h",
    },
    deps=[
        "dep/agg",
        "dep/stb",
        "+fmt_lib",
    ],
)
