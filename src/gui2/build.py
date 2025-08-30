from build.c import cxxprogram, cxxlibrary, simplerule, clibrary
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
    '-DIMHEX_VERSION=\\"\\"',
    "-DIMHEX_PLUGIN_FEATURES_CONTENT={}",
    "-DDEBUG",
]

package(name="dbus_lib", package="dbus-1")
package(name="freetype2_lib", package="freetype2")
package(name="mbedcrypto_lib", package="mbedcrypto")
package(name="libcurl_lib", package="libcurl")
package(name="glfw3_lib", package="glfw3")
package(name="md4c_lib", package="md4c")
package(name="magic_lib", package="libmagic")


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
    ins=[
        f
        for f in glob(
            "src/gui2/rsrc/**",
            recursive=True,
        )
        if isfile(f)
    ],
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
        "dep/libwolv/libs/math_eval/source/math_eval/math_evaluator.cpp",
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

clibrary(name="libmicrotar",
           srcs=sources_from("dep/imhex/lib/third_party/microtar/source"),
           hdrs=headers_from("dep/imhex/lib/third_party/microtar/include"))

cxxlibrary(
    name="libimhex",
    srcs=(
        sources_from("dep/imhex/lib/libimhex/source/ui")
        + sources_from("dep/imhex/lib/libimhex/source/api")
        + sources_from("dep/imhex/lib/libimhex/source/providers")
        + [
            "dep/imhex/lib/libimhex/source/subcommands/subcommands.cpp",
            "dep/imhex/lib/libimhex/source/helpers/crypto.cpp",
            "dep/imhex/lib/libimhex/source/helpers/debugging.cpp",
            ".//default_paths.cpp",
            "dep/imhex/lib/libimhex/source/helpers/fs.cpp",
            "dep/imhex/lib/libimhex/source/helpers/http_requests.cpp",
            "dep/imhex/lib/libimhex/source/helpers/http_requests_native.cpp",
            "dep/imhex/lib/libimhex/source/helpers/encoding_file.cpp",
            "dep/imhex/lib/libimhex/source/helpers/logger.cpp",
            "dep/imhex/lib/libimhex/source/helpers/magic.cpp",
            "dep/imhex/lib/libimhex/source/helpers/opengl.cpp",
            "dep/imhex/lib/libimhex/source/helpers/scaling.cpp",
            "dep/imhex/lib/libimhex/source/helpers/semantic_version.cpp",
            "dep/imhex/lib/libimhex/source/helpers/utils.cpp",
            "dep/imhex/lib/libimhex/source/helpers/utils_linux.cpp",
            "dep/imhex/lib/libimhex/source/helpers/keys.cpp",
            "dep/imhex/lib/libimhex/source/helpers/tar.cpp",
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
        ".+libmicrotar",
        ".+mbedcrypto_lib",
        ".+libcurl_lib",
        ".+glfw3_lib",
        ".+magic_lib",
    ],
)

cxxlibrary(
    name="libtrace",
    srcs=[
        "dep/imhex/lib/trace/source/stacktrace.cpp",
        "dep/imhex/lib/trace/source/exceptions.cpp",
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
    srcs=[
        "dep/imhex/plugins/ui/source/ui/text_editor/editor.cpp",
        "dep/imhex/plugins/ui/source/ui/text_editor/highlighter.cpp",
        "dep/imhex/plugins/ui/source/ui/text_editor/navigate.cpp",
        "dep/imhex/plugins/ui/source/ui/text_editor/render.cpp",
        "dep/imhex/plugins/ui/source/ui/text_editor/support.cpp",
        "dep/imhex/plugins/ui/source/ui/text_editor/utf8.cpp",
        "dep/imhex/plugins/ui/source/ui/hex_editor.cpp",
        "dep/imhex/plugins/ui/source/ui/markdown.cpp",
        "dep/imhex/plugins/ui/source/ui/widgets.cpp",
        "./menu_items.cpp",
    ],
    hdrs=headers_from("dep/imhex/plugins/ui/include") | {},
    cflags=cflags,
    deps=[".+libimhex", ".+libromfs", ".+fonts-plugin", ".+md4c_lib"],
)

cxxlibrary(
    name="builtin-plugin",
    srcs=[
        "dep/imhex/plugins/builtin/source/content/events.cpp",
        "dep/imhex/plugins/builtin/source/content/global_actions.cpp",
        "dep/imhex/plugins/builtin/source/content/init_tasks.cpp",
        "dep/imhex/plugins/builtin/source/content/popups/hex_editor/popup_hex_editor_find.cpp",
        "dep/imhex/plugins/builtin/source/content/differing_byte_searcher.cpp",
        "dep/imhex/plugins/builtin/source/content/providers/file_provider.cpp",
        "dep/imhex/plugins/builtin/source/content/providers/view_provider.cpp",
        "dep/imhex/plugins/builtin/source/content/data_visualizers.cpp",
        "dep/imhex/plugins/builtin/source/content/settings_entries.cpp",
        "dep/imhex/plugins/builtin/source/content/themes.cpp",
        "dep/imhex/plugins/builtin/source/content/ui_items.cpp",
        "dep/imhex/plugins/builtin/source/content/project.cpp",
        "dep/imhex/plugins/builtin/source/content/workspaces.cpp",
        "dep/imhex/plugins/builtin/source/content/views/view_hex_editor.cpp",
        "dep/imhex/plugins/builtin/source/content/views/view_logs.cpp",
        "dep/imhex/plugins/builtin/source/content/views/view_theme_manager.cpp",
        "dep/imhex/plugins/builtin/source/content/views/view_settings.cpp",
        "dep/imhex/plugins/builtin/source/content/views/view_about.cpp",
        "dep/imhex/plugins/builtin/source/content/window_decoration.cpp",
        "dep/imhex/plugins/builtin/source/plugin_builtin.cpp",
    ],
    hdrs=headers_from("dep/imhex/plugins/builtin/include"),
    cflags=cflags,
    deps=[
        ".+libimhex",
        ".+libromfs",
        ".+libtrace",
        ".+libpl",
        ".+libwolv",
        ".+ui-plugin",
        ".+fonts-plugin",
    ],
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
        "./main_menu_items.cc",
        "./customview.cc",
        "./customview.h",
        ".+romfs",
    ],
    cflags=cflags,
    ldflags=["-ldl"],
    deps=[
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
