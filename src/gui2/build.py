from build.c import cxxprogram, cxxlibrary, simplerule
from build.pkg import package
from glob import glob
from functools import reduce
import operator
from os.path import *

cflags = [
    '-DIMHEX_PROJECT_NAME=\\"fluxengine\\"',
    "-DOS_LINUX",
    "-DLIBROMFS_PROJECT_NAME=fluxengine",
    "-DIMHEX_STATIC_LINK_PLUGINS",
]

package(name="dbus_lib", package="dbus-1")
package(name="freetype2_lib", package="freetype2")
package(name="mbedcrypto_lib", package="mbedcrypto")
package(name="libcurl_lib", package="libcurl")
package(name="glfw3_lib", package="glfw3")


def headers_from(path):
    hdrs = {k: f"{path}/{k}" for k in glob("**/*.h*", root_dir=path, recursive=True)}
    assert hdrs, f"path {path} contained no headers"
    return hdrs


def sources_from(path):
    srcs = [join(path, f) for f in glob("**/*.c*", root_dir=path, recursive=True)]
    assert srcs, f"path {path} contained no sources"
    return srcs


cxxlibrary(
    name="libnfd",
    srcs=[
        "dep/native-file-dialog/src/nfd_portal.cpp",
    ],
    hdrs={
        "nfd.hpp": "dep/native-file-dialog/src/include/nfd.hpp",
        "nfd.h": "dep/native-file-dialog/src/include/nfd.h",
    },
    deps=[".+dbus_lib"],
)

cxxlibrary(
    name="imgui",
    srcs=sources_from("dep/imhex/lib/third_party/imgui/backend/source")
    + sources_from("dep/imhex/lib/third_party/imgui/imnodes/source")
    + sources_from("dep/imhex/lib/third_party/imgui/implot/source")
    + sources_from("dep/imhex/lib/third_party/imgui/implot3d/source")
    + sources_from("dep/imhex/lib/third_party/imgui/imgui/source"),
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
        "imconfig.h": "./imconfig.h",
    },
    deps=[".+freetype2_lib"],
)

cxxlibrary(name="libxdgpp", srcs=[], hdrs={"xdg.hpp": "dep/xdgpp/xdg.hpp"})

cxxlibrary(
    name="libromfs",
    srcs=sources_from("dep/libromfs/lib/source"),
    cflags=cflags,
    hdrs={"romfs/romfs.hpp": "dep/libromfs/lib/include/romfs/romfs.hpp"},
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
    name="libwolv",
    srcs=[
        "dep/libwolv/libs/io/source/io/file.cpp",
        "dep/libwolv/libs/io/source/io/file_unix.cpp",
        "dep/libwolv/libs/io/source/io/fs.cpp",
        "dep/libwolv/libs/io/source/io/handle.cpp",
        "dep/libwolv/libs/utils/source/utils/string.cpp",
    ],
    hdrs=reduce(
        operator.ior,
        [
            headers_from(f"dep/libwolv/libs/{d}/include")
            for d in ["types", "io", "utils", "containers", "hash", "math_eval"]
        ],
    )
    | {"types/uintwide_t.h": "dep/libwolv/libs/types/include/wolv/types/uintwide_t.h"},
)

cxxlibrary(
    name="libthrowingptr", srcs=[], hdrs=headers_from("dep/throwing_ptr/include")
)

cxxlibrary(
    name="libpl",
    srcs=sources_from("dep/pattern-language/lib/source"),
    hdrs=(
        headers_from("dep/pattern-language/lib/include")
        | headers_from("dep/pattern-language/generators/include")
    ),
    deps=[".+libthrowingptr", ".+libwolv"],
)

cxxlibrary(name="hacks", srcs=[], hdrs={"jthread.hpp": "./jthread.hpp"})

cxxlibrary(
    name="libimhex",
    srcs=(
        sources_from("dep/imhex/lib/libimhex/source/ui")
        + sources_from("dep/imhex/lib/libimhex/source/api")
        + [
            "dep/imhex/lib/libimhex/source/subcommands/subcommands.cpp",
            "dep/imhex/lib/libimhex/source/helpers/crypto.cpp",
            "dep/imhex/lib/libimhex/source/helpers/default_paths.cpp",
            "dep/imhex/lib/libimhex/source/helpers/fs.cpp",
            "dep/imhex/lib/libimhex/source/helpers/http_requests.cpp",
            "dep/imhex/lib/libimhex/source/helpers/http_requests_native.cpp",
            "dep/imhex/lib/libimhex/source/helpers/logger.cpp",
            "dep/imhex/lib/libimhex/source/helpers/opengl.cpp",
            "dep/imhex/lib/libimhex/source/helpers/scaling.cpp",
            "dep/imhex/lib/libimhex/source/helpers/semantic_version.cpp",
            "dep/imhex/lib/libimhex/source/helpers/utils.cpp",
            "dep/imhex/lib/libimhex/source/helpers/utils_linux.cpp",
            "dep/imhex/lib/libimhex/source/helpers/keys.cpp",
        ]
    ),
    hdrs=headers_from("dep/imhex/lib/libimhex/include"),
    cflags=cflags + ["-I/usr/include/lunasvg"],
    caller_ldflags=["-llunasvg"],
    deps=[
        ".+libwolv",
        ".+imgui",
        ".+libpl",
        ".+libnfd",
        ".+hacks",
        ".+libxdgpp",
        ".+mbedcrypto_lib",
        ".+libcurl_lib",
        ".+glfw3_lib",
    ],
)

cxxlibrary(
    name="libtrace",
    srcs=[
        "dep/imhex/lib/trace/source/stacktrace.cpp",
        "dep/imhex/lib/third_party/llvm-demangle/source/Demangle.cpp",
        "dep/imhex/lib/third_party/llvm-demangle/source/RustDemangle.cpp",
        "dep/imhex/lib/third_party/llvm-demangle/source/DLangDemangle.cpp",
        "dep/imhex/lib/third_party/llvm-demangle/source/ItaniumDemangle.cpp",
        "dep/imhex/lib/third_party/llvm-demangle/source/MicrosoftDemangle.cpp",
        "dep/imhex/lib/third_party/llvm-demangle/source/MicrosoftDemangleNodes.cpp",
    ],
    hdrs=(
        headers_from("dep/imhex/lib/trace/include")
        | headers_from("dep/imhex/lib/third_party/llvm-demangle/include")
        | {
            "ItaniumNodes.def": "dep/imhex/lib/third_party/llvm-demangle/include/llvm/Demangle/ItaniumNodes.def"
        }
    ),
)

cxxlibrary(
    name="init",
    srcs=[
        "dep/imhex/main/gui/source/init/run/cli.cpp",
        "dep/imhex/main/gui/source/init/run/common.cpp",
        "dep/imhex/main/gui/source/init/run/desktop.cpp",
        "dep/imhex/main/gui/source/init/splash_window.cpp",
        "dep/imhex/main/gui/source/init/tasks.cpp",
    ],
    hdrs=headers_from("dep/imhex/main/gui/include"),
    cflags=cflags,
    deps=[
        ".+libimhex",
        ".+imgui",
        ".+libromfs",
    ],
)


cxxlibrary(
    name="fonts-plugin",
    srcs=sources_from("dep/imhex/plugins/fonts/source"),
    hdrs=headers_from("dep/imhex/plugins/fonts/include"),
    cflags=cflags,
    deps=[".+libimhex", ".+libromfs"],
)

cxxlibrary(
    name="ui-plugin",
    srcs=sources_from("dep/imhex/plugins/ui/source"),
    hdrs=headers_from("dep/imhex/plugins/ui/include") | {},
    cflags=cflags,
    deps=[".+libimhex", ".+libromfs", ".+fonts-plugin"],
)

cxxlibrary(
    name="builtin-plugin",
    srcs=sources_from("dep/imhex/plugins/builtin/source"),
    hdrs=headers_from("dep/imhex/plugins/builtin/include"),
    cflags=cflags,
    deps=[".+libimhex", ".+libromfs", ".+libpl", ".+ui-plugin", ".+fonts-plugin"],
)

cxxprogram(
    name="gui2",
    srcs=[
        "dep/imhex/main/gui/include/crash_handlers.hpp",
        "dep/imhex/main/gui/include/messaging.hpp",
        "dep/imhex/main/gui/include/window.hpp",
        "dep/imhex/main/gui/source/crash_handlers.cpp",
        "dep/imhex/main/gui/source/main.cpp",
        "dep/imhex/main/gui/source/messaging/common.cpp",
        "dep/imhex/main/gui/source/messaging/linux.cpp",
        "dep/imhex/main/gui/source/window/platform/linux.cpp",
        "dep/imhex/main/gui/source/window/window.cpp",
        "./fluxengine.cc",
        ".+romfs",
    ],
    cflags=cflags,
    ldflags=["-ldl"],
    deps=[
        ".+libimhex",
        ".+libpl",
        ".+init",
        ".+libtrace",
        "+fmt_lib",
        ".+hacks",
        ".+builtin-plugin",
        ".+fonts-plugin",
        ".+ui-plugin",
    ],
)
