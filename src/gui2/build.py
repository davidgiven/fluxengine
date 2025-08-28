from build.c import cxxprogram, cxxlibrary
from glob import glob
from functools import reduce
import operator

cxxlibrary(
    name="imgui",
    srcs=[],
    hdrs={
        k: f"dep/imgui/{k}"
        for k in glob("**/*.h", root_dir="dep/imgui", recursive=True)
    },
)

cxxlibrary(
    name="libwolv",
    srcs=[],
    hdrs=reduce(
        operator.ior,
        [
            {
                k: f"dep/libwolv/libs/{d}/include/{k}"
                for k in glob(
                    "**/*.hpp", root_dir=f"dep/libwolv/libs/{d}/include", recursive=True
                )
            }
            for d in ["types", "io", "utils"]
        ],
    )
    | {"types/uintwide_t.h": "dep/libwolv/libs/types/include/wolv/types/uintwide_t.h"},
)

cxxlibrary(
    name="libimhex",
    srcs=[],
    hdrs={
        k: f"dep/imhex/lib/libimhex/include/{k}"
        for k in glob(
            "**/*.h*", root_dir="dep/imhex/lib/libimhex/include", recursive=True
        )
    },
    deps=[".+libwolv", ".+imgui"],
)

cxxprogram(
    name="gui2",
    srcs=[
        "dep/imhex/main/gui/include/window.hpp",
        "dep/imhex/main/gui/source/main.cpp",
        "dep/imhex/main/gui/source/crash_handlers.cpp",
    ],
    deps=[".+libimhex"],
)
