from build.c import cxxlibrary

cxxlibrary(
    name="algorithms",
    srcs=["./readerwriter.cc"],
    hdrs={
        "lib/algorithms/readerwriter.h": "./readerwriter.h",
    },
    deps=[
        "lib/core",
        "lib/config",
        "lib/data",
        "lib/usb",
        "lib/encoders",
        "lib/decoders",
        "lib/fluxsource",
        "lib/fluxsink",
        "lib/imagereader",
        "lib/imagewriter",
    ],
)
