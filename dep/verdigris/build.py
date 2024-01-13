from build.c import clibrary

clibrary(
    name="verdigris",
    hdrs={
        "wobjectcpp.h": "./src/wobjectcpp.h",
        "wobjectdefs.h": "./src/wobjectdefs.h",
        "wobjectimpl.h": "./src/wobjectimpl.h",
    },
)
