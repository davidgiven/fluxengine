from build.c import cxxlibrary

cxxlibrary(
    name="data",
    srcs=[
        "./fluxmap.cc",
        "./sector.cc",
        "./layout.cc",
        "./image.cc",
        "./fluxmapreader.cc",
        "./fluxpattern.cc",
    ],
    hdrs={
        "lib/data/flux.h": "./flux.h",
        "lib/data/fluxmap.h": "./fluxmap.h",
        "lib/data/sector.h": "./sector.h",
        "lib/data/layout.h": "./layout.h",
        "lib/data/image.h": "./image.h",
        "lib/data/fluxmapreader.h": "./fluxmapreader.h",
        "lib/data/fluxpattern.h": "./fluxpattern.h",
    },
    deps=["lib/core", "lib/config", "+protocol"],
)
