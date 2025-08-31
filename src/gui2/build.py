from build.c import cxxprogram, cxxlibrary, simplerule, clibrary
from build.pkg import package
from build.utils import filenamesmatchingof
from glob import glob
from functools import reduce
import operator
from os.path import *

cflags = [
    "--std=c++23",
    '-DIMHEX_PROJECT_NAME=\\"fluxengine\\"',
    "-DLIBROMFS_PROJECT_NAME=fluxengine",
    "-fpic",
]


def headers_from(path):
    hdrs = {k: f"{path}/{k}" for k in glob("**/*.h*", root_dir=path, recursive=True)}
    assert hdrs, f"path {path} contained no headers"
    return hdrs


def sources_from(path):
    srcs = [join(path, f) for f in glob("**/*.c*", root_dir=path, recursive=True)]
    assert srcs, f"path {path} contained no sources"
    return srcs


cxxlibrary(
    name="libimgui",
    srcs=[],
    hdrs=reduce(
        operator.ior,
        [
            headers_from(f"dep/imhex/lib/third_party/imgui/{d}/include")
            for d in [
                "imgui",
                "backend",
                "implot",
                "implot3d",
                "imnodes",
                "cimgui",
            ]
        ],
    )
    | {
        "imgui_freetype.h": "dep/imhex/lib/third_party/imgui/imgui/include/misc/freetype/imgui_freetype.h",
    },
    cflags=cflags,
)

cxxlibrary(
    name="libjson",
    srcs=[],
    hdrs=headers_from("dep/imhex/lib/third_party/nlohmann_json/include"),
    cflags=cflags,
)

cxxlibrary(
    name="libromfs",
    srcs=sources_from("dep/libromfs/lib/source"),
    hdrs=headers_from("dep/libromfs/lib/include"),
    cflags=cflags,
)

cxxlibrary(
    name="libwolv",
    srcs=[],
    hdrs=reduce(
        operator.ior,
        [
            headers_from(f"dep/libwolv/libs/{d}/include")
            for d in ["types", "io", "utils", "containers", "hash", "math_eval"]
        ],
    ),
    cflags=cflags,
)

cxxlibrary(
    name="libimhex",
    srcs=[],
    hdrs=headers_from("dep/imhex/lib/libimhex/include"),
    deps=[".+libwolv", ".+libimgui", ".+libjson"],
    cflags=cflags,
)

cxxprogram(
    name="mkromfs",
    srcs=sources_from("dep/libromfs/generator/source"),
    cflags=[
        '-DLIBROMFS_PROJECT_NAME=\\"fluxengine\\"',
        '-DRESOURCE_LOCATION=\\"rsrc\\"',
    ],
)

simplerule(
    name="romfs",
    ins=glob("src/gui2/rsrc/*", recursive=True),
    outs=["=romfs.cc"],
    deps=[".+mkromfs"],
    commands=[
        "ln -s $$(dirname $$(realpath $[ins[0]])) rsrc",
        "$[deps[0]]",
        "mv libromfs_resources.cpp $[outs[0]]",
    ],
    label="ROMFS",
)

cxxlibrary(
    name="plugin_lib",
    srcs=["./plugin.cc", ".+romfs"],
    deps=[".+libimhex", ".+libromfs"],
    cflags=cflags,
)

simplerule(
    name="plugin",
    ins=[".+plugin_lib", ".+libromfs"],
    outs=["=plugin.hexplug"],
    commands=[
        "$(CC) -shared -o $[outs[0]] -Wl,--whole-archive $[[f for f in filenamesof(ins) if f.endswith('.a')]] -Wl,--no-whole-archive"
    ],
)
