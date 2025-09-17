from build.c import cxxprogram, cxxlibrary, simplerule, clibrary
from build.ab import targetof, Rule, Target
from build.pkg import package
from build.utils import filenamesmatchingof
from glob import glob
from functools import reduce
import operator
from os.path import *
import config

cflags = [
    '-DIMHEX_PROJECT_NAME=\\"fluxengine\\"',
    "-DIMHEX_STATIC_LINK_PLUGINS",
    '-DIMHEX_VERSION=\\"0.0.0\\"',
    "-DIMHEX_PLUGIN_FEATURES_CONTENT={}",
    # "-DDEBUG",
]
if config.osx:
    cflags = cflags + ["-DOS_MACOS"]
elif config.windows:
    cflags = cflags + ["-DOS_WINDOWS"]
else:
    cflags = cflags + ["-DOS_LINUX"]

package(name="dbus_lib", package="dbus-1")
package(name="freetype2_lib", package="freetype2")
package(name="libcurl_lib", package="libcurl")
package(name="glfw3_lib", package="glfw3")
package(name="md4c_lib", package="md4c")
package(name="magic_lib", package="libmagic")
package(name="nlohmannjson_lib", package="nlohmann_json")


def headers_from(path):
    hdrs = {k: f"{path}/{k}" for k in glob("**/*.h*", root_dir=path, recursive=True)}
    assert hdrs, f"path {path} contained no headers"
    return hdrs


def sources_from(path, except_for=[]):
    srcs = [join(path, f) for f in glob("**/*.[ch]*", root_dir=path, recursive=True)]
    srcs = [f for f in srcs if f not in except_for]
    assert srcs, f"path {path} contained no sources"
    return srcs


if config.osx:
    clibrary(
        name="libnfd",
        srcs=["dep/native-file-dialog/src/nfd_cocoa.m"],
        hdrs={
            "nfd.hpp": "dep/native-file-dialog/src/include/nfd.hpp",
            "nfd.h": "dep/native-file-dialog/src/include/nfd.h",
        },
    )
elif config.windows:
    cxxlibrary(
        name="libnfd",
        srcs=(["dep/native-file-dialog/src/nfd_win.cpp"]),
        hdrs={
            "nfd.hpp": "dep/native-file-dialog/src/include/nfd.hpp",
            "nfd.h": "dep/native-file-dialog/src/include/nfd.h",
        },
    )
else:
    cxxlibrary(
        name="libnfd",
        srcs=(["dep/native-file-dialog/src/nfd_portal.cpp"]),
        hdrs={
            "nfd.hpp": "dep/native-file-dialog/src/include/nfd.hpp",
            "nfd.h": "dep/native-file-dialog/src/include/nfd.h",
        },
        deps=[".+dbus_lib"],
    )

clibrary(
    name="plutovg",
    srcs=sources_from("dep/lunasvg/plutovg/source"),
    hdrs=headers_from("dep/lunasvg/plutovg/include"),
)

cxxlibrary(
    name="lunasvg",
    srcs=sources_from("dep/lunasvg/source"),
    hdrs=headers_from("dep/lunasvg/include"),
    deps=[".+plutovg"],
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
        "imconfig.h": "./imhex_overrides/imconfig.h",
    },
    deps=[".+freetype2_lib", ".+lunasvg"],
)

cxxlibrary(name="libxdgpp", srcs=[], hdrs={"xdg.hpp": "dep/xdgpp/xdg.hpp"})

cxxprogram(
    name="mkromfs",
    srcs=["./tools/mkromfs.cc"],
    cflags=[
        '-DLIBROMFS_PROJECT_NAME=\\"fluxengine\\"',
        '-DRESOURCE_LOCATION=\\"rsrc\\"',
    ],
    deps=["+fmt_lib"],
)

if config.osx:
    clibrary(
        name="libwolv-io-fs",
        srcs=[
            "dep/libwolv/libs/io/source/io/fs_macos.m",
        ],
        cflags=cflags,
    )
elif config.windows:
    cxxlibrary(
        name="libwolv-io-fs",
        srcs=["dep/libwolv/libs/io/source/io/file_win.cpp"],
        hdrs=headers_from("dep/libwolv/libs/io/include"),
        cflags=cflags,
    )
else:
    cxxlibrary(
        name="libwolv-io-fs",
        srcs=["dep/libwolv/libs/io/source/io/file_unix.cpp"],
        hdrs=headers_from("dep/libwolv/libs/io/include"),
        cflags=cflags,
    )

wolv_modules = ["types", "io", "utils", "containers", "hash", "math_eval", "net"]
cxxlibrary(
    name="libwolv",
    srcs=(
        [
            "dep/libwolv/libs/io/source/io/file.cpp",
            "dep/libwolv/libs/io/source/io/file_unix.cpp",
            "dep/libwolv/libs/io/source/io/fs.cpp",
            "dep/libwolv/libs/io/source/io/handle.cpp",
            "dep/libwolv/libs/math_eval/source/math_eval/math_evaluator.cpp",
            "dep/libwolv/libs/utils/source/utils/string.cpp",
        ]
        + sources_from("dep/libwolv/libs/net/source")
    ),
    hdrs=reduce(
        operator.ior,
        [headers_from(f"dep/libwolv/libs/{d}/include") for d in wolv_modules],
    )
    | {"types/uintwide_t.h": "dep/libwolv/libs/types/include/wolv/types/uintwide_t.h"},
    deps=[".+libwolv-io-fs"],
    cflags=cflags
)

cxxlibrary(
    name="libthrowingptr", srcs=[], hdrs=headers_from("dep/throwing_ptr/include")
)

cxxlibrary(
    name="libpl",
    srcs=sources_from("dep/pattern-language/lib/source")
    + sources_from("dep/pattern-language/cli/source"),
    hdrs=(
        headers_from("dep/pattern-language/lib/include")
        | headers_from("dep/pattern-language/generators/include")
        | headers_from("dep/pattern-language/cli/include")
    ),
    deps=[".+libthrowingptr", ".+libwolv"],
)

cxxlibrary(name="hacks", srcs=[], hdrs={"jthread.hpp": "./imhex_overrides/jthread.hpp"})

clibrary(
    name="libmicrotar",
    srcs=sources_from("dep/imhex/lib/third_party/microtar/source"),
    hdrs=headers_from("dep/imhex/lib/third_party/microtar/include"),
)

if config.osx:
    clibrary(
        name="libimhex-utils",
        srcs=[
            "dep/imhex/lib/libimhex/source/helpers/utils_macos.m",
            "dep/imhex/lib/libimhex/source/helpers/macos_menu.m",
        ],
        hdrs=headers_from("dep/imhex/lib/libimhex/include"),
        cflags=cflags,
    )
elif config.unix:
    cxxlibrary(
        name="libimhex-utils",
        srcs=["dep/imhex/lib/libimhex/source/helpers/utils_linux.cpp"],
        hdrs=headers_from("dep/imhex/lib/libimhex/include"),
        cflags=cflags,
    )

cxxlibrary(
    name="libimhex",
    srcs=(
        sources_from("dep/imhex/lib/libimhex/source/ui")
        + sources_from(
            "dep/imhex/lib/libimhex/source/api",
            except_for=["dep/imhex/lib/libimhex/source/api/achievement_manager.cpp"],
        )
        + sources_from("dep/imhex/lib/libimhex/source/data_processor")
        + sources_from("dep/imhex/lib/libimhex/source/providers")
        + [
            "dep/imhex/lib/libimhex/source/subcommands/subcommands.cpp",
            "dep/imhex/lib/libimhex/source/helpers/crypto.cpp",
            "dep/imhex/lib/libimhex/source/helpers/debugging.cpp",
            "./imhex_overrides/achievement_manager.cpp",
            "./imhex_overrides/default_paths.cpp",
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
            "dep/imhex/lib/libimhex/source/helpers/patches.cpp",
            "dep/imhex/lib/libimhex/source/helpers/keys.cpp",
            "dep/imhex/lib/libimhex/source/helpers/tar.cpp",
            "dep/imhex/lib/libimhex/source/helpers/udp_server.cpp",
        ],
    ),
    hdrs=headers_from("dep/imhex/lib/libimhex/include"),
    cflags=cflags,
    deps=[
        ".+libwolv",
        ".+imgui",
        ".+libpl",
        ".+libnfd",
        ".+libxdgpp",
        ".+libmicrotar",
        ".+libimhex-utils",
        ".+libcurl_lib",
        ".+glfw3_lib",
        ".+magic_lib",
        ".+hacks",
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


def romfs(name, id, dir):
    cxxprogram(
        name=f"{id}_mkromfs",
        srcs=["./tools/mkromfs.cc"],
        cflags=[
            f'-DLIBROMFS_PROJECT_NAME=\\"{id}\\"',
            f'-DRESOURCE_LOCATION=\\"{dir}\\"',
        ],
        deps=["+fmt_lib"],
    )

    simplerule(
        name=name,
        ins=[f for f in glob(dir + "/**", recursive=True) if isfile(f)],
        outs=["=romfs.cc"],
        deps=[f".+{id}_mkromfs"],
        commands=[
            f"$[deps[0]] {dir}",
            "mv libromfs_resources.cpp $[outs[0]]",
        ],
        label="ROMFS",
    )


def plugin(name, id, srcs, hdrs, romfsdir, deps):
    romfs(name=f"{id}_romfs", id=id, dir=romfsdir)

    cxxlibrary(
        name=name,
        srcs=srcs + [f".+{id}_romfs", "dep/libromfs/lib/source/romfs.cpp"],
        hdrs=hdrs | headers_from("dep/libromfs/lib/include"),
        cflags=cflags
        + [
            f"-DIMHEX_PLUGIN_NAME={id}",
            f"-DLIBROMFS_PROJECT_NAME={id}",
            f"-Dromfs=romfs_{id}",
        ],
        deps=deps,
    )


plugin(
    name="fonts-plugin",
    id="fonts",
    srcs=sources_from("dep/imhex/plugins/fonts/source"),
    hdrs=headers_from("dep/imhex/plugins/fonts/include"),
    romfsdir="dep/imhex/plugins/fonts/romfs",
    deps=[".+libimhex"],
)

plugin(
    name="ui-plugin",
    id="ui",
    srcs=(
        sources_from("dep/imhex/plugins/ui/source/ui/text_editor")
        + [
            "dep/imhex/plugins/ui/source/ui/hex_editor.cpp",
            "dep/imhex/plugins/ui/source/ui/markdown.cpp",
            "dep/imhex/plugins/ui/source/ui/pattern_drawer.cpp",
            "dep/imhex/plugins/ui/source/ui/pattern_value_editor.cpp",
            "dep/imhex/plugins/ui/source/ui/visualizer_drawer.cpp",
            "dep/imhex/plugins/ui/source/ui/widgets.cpp",
            "dep/imhex/plugins/ui/source/library_ui.cpp",
            "./imhex_overrides/menu_items.cpp",
        ]
    ),
    hdrs=headers_from("dep/imhex/plugins/ui/include"),
    romfsdir="dep/imhex/plugins/ui/romfs",
    deps=[".+libimhex", ".+fonts-plugin", ".+md4c_lib"],
)

plugin(
    name="builtin-plugin",
    id="builtin",
    srcs=sources_from(
        "dep/imhex/plugins/builtin/source",
        except_for=[
            "dep/imhex/plugins/builtin/source/content/views/view_achievements.cpp",
            "dep/imhex/plugins/builtin/source/content/achievements.cpp",
            "dep/imhex/plugins/builtin/source/content/main_menu_items.cpp",
            "dep/imhex/plugins/builtin/source/content/welcome_screen.cpp",
            "dep/imhex/plugins/builtin/source/content/out_of_box_experience.cpp",
            "dep/imhex/plugins/builtin/source/content/views.cpp",
            "dep/imhex/plugins/builtin/source/content/views/view_data_processor.cpp",
            "dep/imhex/plugins/builtin/source/content/views/view_tutorials.cpp",
            "dep/imhex/plugins/builtin/source/content/data_processor_nodes.cpp",
        ]
        + glob("dep/imhex/plugins/builtin/source/content/data_processor_nodes/*")
        + glob("dep/imhex/plugins/builtin/source/content/tutorials/*"),
    )
    + [
        "./imhex_overrides/welcome.cc",
        "./imhex_overrides/stubs.cc",
        "./imhex_overrides/views.cpp",
        "./imhex_overrides/main_menu_items.cpp",
    ],
    hdrs=headers_from("dep/imhex/plugins/builtin/include"),
    romfsdir="dep/imhex/plugins/builtin/romfs",
    deps=[
        ".+libimhex",
        ".+libtrace",
        ".+libpl",
        ".+libwolv",
        ".+ui-plugin",
        ".+fonts-plugin",
    ],
)

plugin(
    name="gui-plugin",
    id="gui",
    srcs=[
        "./imhex_overrides/splash_window.cpp",
        "dep/imhex/main/gui/include/crash_handlers.hpp",
        "dep/imhex/main/gui/include/messaging.hpp",
        "dep/imhex/main/gui/include/window.hpp",
        "dep/imhex/main/gui/source/crash_handlers.cpp",
        "dep/imhex/main/gui/source/init/run/cli.cpp",
        "dep/imhex/main/gui/source/init/run/common.cpp",
        "dep/imhex/main/gui/source/init/run/desktop.cpp",
        "dep/imhex/main/gui/source/init/tasks.cpp",
        "dep/imhex/main/gui/source/main.cpp",
        "dep/imhex/main/gui/source/messaging/common.cpp",
        "dep/imhex/main/gui/source/window/window.cpp",
    ]
    + (
        [
            "dep/imhex/main/gui/source/window/platform/macos.cpp",
            "dep/imhex/main/gui/source/messaging/macos.cpp",
        ]
        if config.osx
        else (
            ["dep/imhex/main/gui/source/window/platform/windows.cpp"]
            if config.windows
            else [
                "dep/imhex/main/gui/source/window/platform/linux.cpp",
                "dep/imhex/main/gui/source/messaging/linux.cpp",
            ]
        )
    ),
    hdrs=headers_from("dep/imhex/main/gui/include"),
    romfsdir="src/gui2/imhex_overrides/rsrc",
    deps=[
        ".+libtrace",
        ".+libimhex",
        ".+imgui",
    ],
)

plugin(
    name="fluxengine-plugin",
    id="fluxengine",
    srcs=[
        "./abstractsectorview.cc",
        "./abstractsectorview.h",
        "./configview.cc",
        "./configview.h",
        "./controlpanelview.cc",
        "./controlpanelview.h",
        "./datastore.cc",
        "./datastore.h",
        "./diskprovider.cc",
        "./diskprovider.h",
        "./fluxengine.cc",
        "./globals.h",
        "./imageview.cc",
        "./imageview.h",
        "./physicalview.cc",
        "./physicalview.h",
        "./summaryview.cc",
        "./summaryview.h",
        "./utils.cc",
        "./utils.h",
    ],
    hdrs={},
    romfsdir="src/gui2/rsrc",
    deps=[
        ".+libimhex",
        ".+fonts-plugin",
        ".+ui-plugin",
        "+protocol",
        "dep/adflib",
        "dep/fatfs",
        "dep/hfsutils",
        "dep/libusbp",
        "lib/core",
        "lib/data",
        "lib/vfs",
        "lib/config",
        "lib/usb",
        "src/gui/drivetypes",
        "src/formats",
        "+z_lib",
    ],
)

cxxprogram(
    name="gui2",
    srcs=[
        "./main.cc",
    ],
    cflags=cflags,
    ldflags=["-ldl", "-lmbedcrypto"],
    deps=[
        ".+libpl",
        "+fmt_lib",
        ".+builtin-plugin",
        ".+fonts-plugin",
        ".+ui-plugin",
        ".+gui-plugin",
        ".+fluxengine-plugin",
    ],
)
