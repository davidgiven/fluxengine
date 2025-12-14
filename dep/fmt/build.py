from build.c import cxxlibrary

cxxlibrary(
    name="fmt",
    srcs=[
        "./src/format.cc",
        "./src/os.cc",
    ],
    cflags=["-Idep/fmt/include"],
    hdrs={
        "fmt/args.h": "./include/fmt/args.h",
        "fmt/base.h": "./include/fmt/base.h",
        "fmt/chrono.h": "./include/fmt/chrono.h",
        "fmt/color.h": "./include/fmt/color.h",
        "fmt/compile.h": "./include/fmt/compile.h",
        "fmt/core.h": "./include/fmt/core.h",
        "fmt/format.h": "./include/fmt/format.h",
        "fmt/format-inl.h": "./include/fmt/format-inl.h",
        "fmt/os.h": "./include/fmt/os.h",
        "fmt/ostream.h": "./include/fmt/ostream.h",
        "fmt/printf.h": "./include/fmt/printf.h",
        "fmt/ranges.h": "./include/fmt/ranges.h",
        "fmt/std.h": "./include/fmt/std.h",
        "fmt/xchar.h": "./include/fmt/xchar.h",
    },
)
