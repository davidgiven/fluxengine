from build.c import cxxprogram, cxxlibrary, simplerule, clibrary
from build.ab import simplerule
from build.pkg import package
from build.utils import glob
from os.path import *
import config

cflags = [
    '-DIMHEX_PROJECT_NAME=\\"fluxengine\\"',
    "-DIMHEX_STATIC_LINK_PLUGINS",
    '-DIMHEX_VERSION=\\"0.0.0\\"',
    "-DUNICODE",
    # "-DDEBUG",
]
if config.osx:
    cflags = cflags + ["-DOS_MACOS"]
elif config.windows:
    cflags = cflags + ["-DOS_WINDOWS"]
else:
    cflags = cflags + ["-DOS_LINUX"]


package(name="freetype2_lib", package="freetype2")
package(name="libcurl_lib", package="libcurl")
package(name="glfw3_lib", package="glfw3")
package(name="magic_lib", package="libmagic")
package(name="mbedtls_lib", package="mbedtls")

cxxlibrary(
    name="imgui",
    srcs=[
        "dep/r/imhex/lib/third_party/imgui/imgui_test_engine/source/imgui_te_perftool.cpp",
        "dep/r/imhex/lib/third_party/imgui/imgui_test_engine/source/imgui_te_coroutine.cpp",
        "dep/r/imhex/lib/third_party/imgui/imgui_test_engine/source/imgui_te_exporters.cpp",
        "dep/r/imhex/lib/third_party/imgui/imgui_test_engine/source/imgui_te_context.cpp",
        "dep/r/imhex/lib/third_party/imgui/imgui_test_engine/source/imgui_capture_tool.cpp",
        "dep/r/imhex/lib/third_party/imgui/imgui_test_engine/source/imgui_te_utils.cpp",
        "dep/r/imhex/lib/third_party/imgui/imgui_test_engine/source/imgui_te_ui.cpp",
        "dep/r/imhex/lib/third_party/imgui/imgui_test_engine/source/imgui_te_engine.cpp",
        "dep/r/imhex/lib/third_party/imgui/backend/source/imgui_impl_glfw.cpp",
        "dep/r/imhex/lib/third_party/imgui/backend/source/imgui_impl_opengl3.cpp",
        "dep/r/imhex/lib/third_party/imgui/imgui/source/imgui.cpp",
        "dep/r/imhex/lib/third_party/imgui/imgui/source/imgui_demo.cpp",
        "dep/r/imhex/lib/third_party/imgui/imgui/source/imgui_draw.cpp",
        "dep/r/imhex/lib/third_party/imgui/imgui/source/imgui_tables.cpp",
        "dep/r/imhex/lib/third_party/imgui/imgui/source/imgui_widgets.cpp",
        "dep/r/imhex/lib/third_party/imgui/imgui/source/misc/freetype/imgui_freetype.cpp",
        "dep/r/imhex/lib/third_party/imgui/imnodes/source/imnodes.cpp",
        "dep/r/imhex/lib/third_party/imgui/implot/source/implot.cpp",
        "dep/r/imhex/lib/third_party/imgui/implot/source/implot_demo.cpp",
        "dep/r/imhex/lib/third_party/imgui/implot/source/implot_items.cpp",
        "dep/r/imhex/lib/third_party/imgui/implot3d/source/implot3d.cpp",
        "dep/r/imhex/lib/third_party/imgui/implot3d/source/implot3d_demo.cpp",
        "dep/r/imhex/lib/third_party/imgui/implot3d/source/implot3d_items.cpp",
        "dep/r/imhex/lib/third_party/imgui/implot3d/source/implot3d_meshes.cpp",
    ],
    hdrs={
        "cimgui.h": "dep/r/imhex/lib/third_party/imgui/cimgui/include/cimgui.h",
        "imconfig.h": "./imhex_overrides/imconfig.h",
        "misc/freetype/imgui_freetype.h": "dep/r/imhex/lib/third_party/imgui/imgui/include/misc/freetype/imgui_freetype.h",
        "imgui_freetype.h": "dep/r/imhex/lib/third_party/imgui/imgui/include/misc/freetype/imgui_freetype.h",
    }
    | {
        f: f"dep/r/imhex/lib/third_party/imgui/imgui/include/{f}"
        for f in [
            "imstb_truetype.h",
            "imconfig.h",
            "imgui.h",
            "imstb_rectpack.h",
            "imgui_internal.h",
            "imstb_textedit.h",
        ]
    }
    | {
        f: f"dep/r/imhex/lib/third_party/imgui/backend/include/{f}"
        for f in [
            "imgui_impl_glfw.h",
            "imgui_impl_opengl3.h",
            "imgui_impl_opengl3_loader.h",
            "emscripten_browser_clipboard.h",
            "opengl_support.h",
            "stb_image.h",
        ]
    }
    | {
        f: f"dep/r/imhex/lib/third_party/imgui/implot/include/{f}"
        for f in [
            "implot.h",
            "implot_internal.h",
        ]
    }
    | {
        f: f"dep/r/imhex/lib/third_party/imgui/implot3d/include/{f}"
        for f in [
            "implot3d.h",
            "implot3d_internal.h",
        ]
    }
    | {
        f: f"dep/r/imhex/lib/third_party/imgui/imnodes/include/{f}"
        for f in [
            "imnodes.h",
            "imnodes_internal.h",
        ]
    }
    | {
        f: f"dep/r/imhex/lib/third_party/imgui/imgui_test_engine/include/{f}"
        for f in [
            "imgui_capture_tool.h",
            "imgui_te_context.h",
            "thirdparty/stb/imstb_image_write.h",
            "thirdparty/Str/Str.h",
            "thirdparty/Str/Str.natvis",
            "imgui_te_utils.h",
            "imgui_te_engine.h",
            "imgui_te_internal.h",
            "imgui_te_ui.h",
            "imgui_te_exporters.h",
            "imgui_te_imconfig.h",
            "imgui_te_perftool.h",
            "imgui_te_coroutine.h",
        ]
    },
    deps=[".+freetype2_lib", "dep+lunasvg", ".+glfw3_lib", "dep+imhex_repo"],
)

cxxprogram(
    name="mkromfs",
    srcs=["./tools/mkromfs.cc"],
    cflags=[
        '-DLIBROMFS_PROJECT_NAME=\\"fluxengine\\"',
        '-DRESOURCE_LOCATION=\\"rsrc\\"',
    ],
    deps=["dep+fmt_lib"],
)

cxxlibrary(
    name="libimhex_core",
    srcs=[
        "./imhex_overrides/achievement_manager.cpp",
        "./imhex_overrides/default_paths.cpp",
        "dep/r/imhex/lib/libimhex/source/api/content_registry.cpp",
        "dep/r/imhex/lib/libimhex/source/api/event_manager.cpp",
        "dep/r/imhex/lib/libimhex/source/api/imhex_api.cpp",
        "dep/r/imhex/lib/libimhex/source/api/layout_manager.cpp",
        "dep/r/imhex/lib/libimhex/source/api/localization_manager.cpp",
        "dep/r/imhex/lib/libimhex/source/api/plugin_manager.cpp",
        "dep/r/imhex/lib/libimhex/source/api/project_file_manager.cpp",
        "dep/r/imhex/lib/libimhex/source/api/shortcut_manager.cpp",
        "dep/r/imhex/lib/libimhex/source/api/task_manager.cpp",
        "dep/r/imhex/lib/libimhex/source/api/theme_manager.cpp",
        "dep/r/imhex/lib/libimhex/source/api/tutorial_manager.cpp",
        "dep/r/imhex/lib/libimhex/source/api/workspace_manager.cpp",
        "dep/r/imhex/lib/libimhex/source/helpers/crypto.cpp",
        "dep/r/imhex/lib/libimhex/source/helpers/debugging.cpp",
        "dep/r/imhex/lib/libimhex/source/helpers/encoding_file.cpp",
        "dep/r/imhex/lib/libimhex/source/helpers/fs.cpp",
        "dep/r/imhex/lib/libimhex/source/helpers/http_requests.cpp",
        "dep/r/imhex/lib/libimhex/source/helpers/http_requests_native.cpp",
        "dep/r/imhex/lib/libimhex/source/helpers/keys.cpp",
        "dep/r/imhex/lib/libimhex/source/helpers/logger.cpp",
        "dep/r/imhex/lib/libimhex/source/helpers/magic.cpp",
        "dep/r/imhex/lib/libimhex/source/helpers/opengl.cpp",
        "dep/r/imhex/lib/libimhex/source/helpers/patches.cpp",
        "dep/r/imhex/lib/libimhex/source/helpers/scaling.cpp",
        "dep/r/imhex/lib/libimhex/source/helpers/semantic_version.cpp",
        "dep/r/imhex/lib/libimhex/source/helpers/tar.cpp",
        "dep/r/imhex/lib/libimhex/source/helpers/udp_server.cpp",
        "dep/r/imhex/lib/libimhex/source/helpers/utils.cpp",
        "dep/r/imhex/lib/libimhex/source/providers/cached_provider.cpp",
        "dep/r/imhex/lib/libimhex/source/providers/memory_provider.cpp",
        "dep/r/imhex/lib/libimhex/source/providers/provider.cpp",
        "dep/r/imhex/lib/libimhex/source/providers/undo/stack.cpp",
        "dep/r/imhex/lib/libimhex/source/subcommands/subcommands.cpp",
        "dep/r/imhex/lib/libimhex/source/ui/banner.cpp",
        "dep/r/imhex/lib/libimhex/source/ui/imgui_imhex_extensions.cpp",
        "dep/r/imhex/lib/libimhex/source/ui/popup.cpp",
        "dep/r/imhex/lib/libimhex/source/ui/toast.cpp",
        "dep/r/imhex/lib/libimhex/source/ui/view.cpp",
    ],
    hdrs={
        f: f"dep/r/imhex/lib/libimhex/include/{f}"
        for f in [
            "hex.hpp",
            "hex/api/achievement_manager.hpp",
            "hex/api/content_registry/background_services.hpp",
            "hex/api/content_registry/command_palette.hpp",
            "hex/api/content_registry/communication_interface.hpp",
            "hex/api/content_registry/data_formatter.hpp",
            "hex/api/content_registry/data_information.hpp",
            "hex/api/content_registry/data_inspector.hpp",
            "hex/api/content_registry/data_processor.hpp",
            "hex/api/content_registry/diffing.hpp",
            "hex/api/content_registry/disassemblers.hpp",
            "hex/api/content_registry/experiments.hpp",
            "hex/api/content_registry/file_type_handler.hpp",
            "hex/api/content_registry/hashes.hpp",
            "hex/api/content_registry/hex_editor.hpp",
            "hex/api/content_registry/pattern_language.hpp",
            "hex/api/content_registry/provider.hpp",
            "hex/api/content_registry/reports.hpp",
            "hex/api/content_registry/settings.hpp",
            "hex/api/content_registry/tools.hpp",
            "hex/api/content_registry/user_interface.hpp",
            "hex/api/content_registry/views.hpp",
            "hex/api/event_manager.hpp",
            "hex/api/events/events_gui.hpp",
            "hex/api/events/events_interaction.hpp",
            "hex/api/events/events_lifecycle.hpp",
            "hex/api/events/events_provider.hpp",
            "hex/api/events/requests_gui.hpp",
            "hex/api/events/requests_interaction.hpp",
            "hex/api/events/requests_lifecycle.hpp",
            "hex/api/events/requests_provider.hpp",
            "hex/api/imhex_api/bookmarks.hpp",
            "hex/api/imhex_api/fonts.hpp",
            "hex/api/imhex_api/hex_editor.hpp",
            "hex/api/imhex_api/messaging.hpp",
            "hex/api/imhex_api/provider.hpp",
            "hex/api/imhex_api/system.hpp",
            "hex/api/layout_manager.hpp",
            "hex/api/localization_manager.hpp",
            "hex/api/plugin_manager.hpp",
            "hex/api/project_file_manager.hpp",
            "hex/api/shortcut_manager.hpp",
            "hex/api/task_manager.hpp",
            "hex/api/theme_manager.hpp",
            "hex/api/tutorial_manager.hpp",
            "hex/api/workspace_manager.hpp",
            "hex/api_urls.hpp",
            "hex/data_processor/attribute.hpp",
            "hex/data_processor/link.hpp",
            "hex/data_processor/node.hpp",
            "hex/helpers/auto_reset.hpp",
            "hex/helpers/binary_pattern.hpp",
            "hex/helpers/concepts.hpp",
            "hex/helpers/crypto.hpp",
            "hex/helpers/debugging.hpp",
            "hex/helpers/default_paths.hpp",
            "hex/helpers/encoding_file.hpp",
            "hex/helpers/fmt.hpp",
            "hex/helpers/fs.hpp",
            "hex/helpers/http_requests.hpp",
            "hex/helpers/http_requests_emscripten.hpp",
            "hex/helpers/http_requests_native.hpp",
            "hex/helpers/keys.hpp",
            "hex/helpers/literals.hpp",
            "hex/helpers/logger.hpp",
            "hex/helpers/magic.hpp",
            "hex/helpers/menu_items.hpp",
            "hex/helpers/opengl.hpp",
            "hex/helpers/patches.hpp",
            "hex/helpers/scaling.hpp",
            "hex/helpers/semantic_version.hpp",
            "hex/helpers/tar.hpp",
            "hex/helpers/types.hpp",
            "hex/helpers/udp_server.hpp",
            "hex/helpers/utils.hpp",
            "hex/helpers/utils_linux.hpp",
            "hex/helpers/utils_macos.hpp",
            "hex/plugin.hpp",
            "hex/providers/buffered_reader.hpp",
            "hex/providers/cached_provider.hpp",
            "hex/providers/memory_provider.hpp",
            "hex/providers/overlay.hpp",
            "hex/providers/provider.hpp",
            "hex/providers/provider_data.hpp",
            "hex/providers/undo_redo/operations/operation.hpp",
            "hex/providers/undo_redo/operations/operation_group.hpp",
            "hex/providers/undo_redo/stack.hpp",
            "hex/subcommands/subcommands.hpp",
            "hex/test/test_provider.hpp",
            "hex/test/tests.hpp",
            "hex/ui/banner.hpp",
            "hex/ui/imgui_imhex_extensions.h",
            "hex/ui/popup.hpp",
            "hex/ui/toast.hpp",
            "hex/ui/view.hpp",
            "hex/ui/widgets.hpp",
        ]
    },
    cflags=cflags,
    deps=[
        ".+glfw3_lib",
        ".+imgui",
        ".+libcurl_lib",
        "dep+microtar_lib",
        ".+magic_lib",
        ".+mbedtls_lib",
        "dep+libwolv_lib",
        "dep+nativefiledialog_lib",
        "dep+patternlanguage_lib",
        "dep+xdgpp_lib",
        "dep+imhex_repo",
    ],
)


if config.osx:
    clibrary(
        name="libimhex",
        srcs=[
            "dep/r/imhex/lib/libimhex/source/helpers/utils_macos.m",
            "dep/r/imhex/lib/libimhex/source/helpers/macos_menu.m",
        ],
        cflags=cflags,
        deps=[".+libimhex_core"],
    )
elif config.windows:
    cxxlibrary(name="libimhex", srcs=[], deps=[".+libimhex_core"])
elif config.unix:
    cxxlibrary(
        name="libimhex",
        srcs=["dep/r/imhex/lib/libimhex/source/helpers/utils_linux.cpp"],
        cflags=cflags,
        deps=[".+libimhex_core"],
    )

cxxlibrary(
    name="libtrace",
    srcs=[
        "dep/r/imhex/lib/trace/source/stacktrace.cpp",
        "dep/r/imhex/lib/trace/source/exceptions.cpp",
        "dep/r/imhex/lib/third_party/llvm-demangle/source/Demangle.cpp",
        "dep/r/imhex/lib/third_party/llvm-demangle/source/RustDemangle.cpp",
        "dep/r/imhex/lib/third_party/llvm-demangle/source/DLangDemangle.cpp",
        "dep/r/imhex/lib/third_party/llvm-demangle/source/ItaniumDemangle.cpp",
        "dep/r/imhex/lib/third_party/llvm-demangle/source/MicrosoftDemangle.cpp",
        "dep/r/imhex/lib/third_party/llvm-demangle/source/MicrosoftDemangleNodes.cpp",
    ],
    hdrs=(
        {
            f: f"dep/r/imhex/lib/trace/include/{f}"
            for f in ["hex/trace/stacktrace.hpp", "hex/trace/exceptions.hpp"]
        }
        | {
            f: f"dep/r/imhex/lib/third_party/llvm-demangle/include/{f}"
            for f in [
                "llvm/Demangle/Utility.h",
                "llvm/Demangle/DemangleConfig.h",
                "llvm/Demangle/MicrosoftDemangleNodes.h",
                "llvm/Demangle/MicrosoftDemangle.h",
                "llvm/Demangle/ItaniumNodes.def",
                "llvm/Demangle/ItaniumDemangle.h",
                "llvm/Demangle/Demangle.h",
                "llvm/Demangle/StringViewExtras.h",
            ]
        }
        | {
            "ItaniumNodes.def": "dep/r/imhex/lib/third_party/llvm-demangle/include/llvm/Demangle/ItaniumNodes.def",
        }
    ),
    deps=["dep+imhex_repo"],
)


def romfs(name, id, dir):
    cxxprogram(
        name=f"{id}_mkromfs",
        srcs=["./tools/mkromfs.cc"],
        cflags=[
            f'-DLIBROMFS_PROJECT_NAME=\\"{id}\\"',
            f'-DRESOURCE_LOCATION=\\"{dir}\\"',
        ],
        deps=["dep+fmt_lib"],
    )

    simplerule(
        name=name,
        ins=glob(dir=dir),
        outs=["=romfs.cc"],
        deps=[f".+{id}_mkromfs"],
        commands=[
            f"$[deps[0]] $[outs[0]] {dir}",
        ],
        label="ROMFS",
    )


def plugin(name, id, srcs, hdrs, romfsdir, deps):
    romfs(name=f"{id}_romfs", id=id, dir=romfsdir)

    cxxlibrary(
        name=name,
        srcs=srcs + [f".+{id}_romfs", "dep/r/libromfs/lib/source/romfs.cpp"],
        hdrs=hdrs
        | {"romfs/romfs.hpp": "dep/r/libromfs/lib/include/romfs/romfs.hpp"},
        cflags=cflags
        + [
            f"-DIMHEX_PLUGIN_NAME={id}",
            f"-DLIBROMFS_PROJECT_NAME={id}",
            f"-DIMHEX_PLUGIN_FEATURES_CONTENT=",
            f"-Dromfs=romfs_{id}",
        ],
        deps=deps + ["dep+libromfs_repo"],
    )


plugin(
    name="fonts-plugin",
    id="fonts",
    srcs=[
        "dep/r/imhex/plugins/fonts/source/font_settings.cpp",
        "dep/r/imhex/plugins/fonts/source/library_fonts.cpp",
        "dep/r/imhex/plugins/fonts/source/font_loader.cpp",
        "dep/r/imhex/plugins/fonts/source/fonts.cpp",
    ],
    hdrs={
        f: f"dep/r/imhex/plugins/fonts/include/{f}"
        for f in [
            "fonts/vscode_icons.hpp",
            "fonts/fonts.hpp",
            "fonts/tabler_icons.hpp",
            "fonts/blender_icons.hpp",
            "font_settings.hpp",
        ]
    },
    romfsdir="dep/r/imhex/plugins/fonts/romfs",
    deps=[".+libimhex"],
)

plugin(
    name="ui-plugin",
    id="ui",
    srcs=[
        "dep/r/imhex/plugins/ui/source/ui/text_editor/highlighter.cpp",
        "dep/r/imhex/plugins/ui/source/ui/text_editor/support.cpp",
        "dep/r/imhex/plugins/ui/source/ui/text_editor/navigate.cpp",
        "dep/r/imhex/plugins/ui/source/ui/text_editor/utf8.cpp",
        "dep/r/imhex/plugins/ui/source/ui/text_editor/render.cpp",
        "dep/r/imhex/plugins/ui/source/ui/text_editor/editor.cpp",
        "dep/r/imhex/plugins/ui/source/ui/hex_editor.cpp",
        "dep/r/imhex/plugins/ui/source/ui/markdown.cpp",
        "dep/r/imhex/plugins/ui/source/ui/pattern_drawer.cpp",
        "dep/r/imhex/plugins/ui/source/ui/pattern_value_editor.cpp",
        "dep/r/imhex/plugins/ui/source/ui/visualizer_drawer.cpp",
        "dep/r/imhex/plugins/ui/source/ui/widgets.cpp",
        "dep/r/imhex/plugins/ui/source/library_ui.cpp",
        "./imhex_overrides/menu_items.cpp",
    ],
    hdrs={
        f: f"dep/r/imhex/plugins/ui/include/{f}"
        for f in [
            "toasts/toast_notification.hpp",
            "popups/popup_notification.hpp",
            "popups/popup_question.hpp",
            "popups/popup_text_input.hpp",
            "popups/popup_file_chooser.hpp",
            "ui/hex_editor.hpp",
            "ui/widgets.hpp",
            "ui/pattern_value_editor.hpp",
            "ui/pattern_drawer.hpp",
            "ui/text_editor.hpp",
            "ui/visualizer_drawer.hpp",
            "ui/markdown.hpp",
            "banners/banner_icon.hpp",
            "banners/banner_button.hpp",
        ]
    },
    romfsdir="dep/r/imhex/plugins/ui/romfs",
    deps=[".+libimhex", ".+fonts-plugin", "dep+md4c_lib"],
)

plugin(
    name="builtin-plugin",
    id="builtin",
    srcs=[
        "./imhex_overrides/main_menu_items.cpp",
        "./imhex_overrides/providers.cpp",
        "./imhex_overrides/stubs.cc",
        "./imhex_overrides/ui_items.cc",
        "./imhex_overrides/views.cpp",
        "./imhex_overrides/welcome.cc",
        "dep/r/imhex/plugins/builtin/source/content/background_services.cpp",
        "dep/r/imhex/plugins/builtin/source/content/command_line_interface.cpp",
        "dep/r/imhex/plugins/builtin/source/content/command_palette_commands.cpp",
        "dep/r/imhex/plugins/builtin/source/content/communication_interface.cpp",
        "dep/r/imhex/plugins/builtin/source/content/data_formatters.cpp",
        "dep/r/imhex/plugins/builtin/source/content/data_information_sections.cpp",
        "dep/r/imhex/plugins/builtin/source/content/data_inspector.cpp",
        "dep/r/imhex/plugins/builtin/source/content/data_visualizers.cpp",
        "dep/r/imhex/plugins/builtin/source/content/differing_byte_searcher.cpp",
        "dep/r/imhex/plugins/builtin/source/content/events.cpp",
        "dep/r/imhex/plugins/builtin/source/content/file_extraction.cpp",
        "dep/r/imhex/plugins/builtin/source/content/file_handlers.cpp",
        "dep/r/imhex/plugins/builtin/source/content/global_actions.cpp",
        "dep/r/imhex/plugins/builtin/source/content/init_tasks.cpp",
        "dep/r/imhex/plugins/builtin/source/content/minimap_visualizers.cpp",
        "dep/r/imhex/plugins/builtin/source/content/pl_builtin_functions.cpp",
        "dep/r/imhex/plugins/builtin/source/content/pl_builtin_types.cpp",
        "dep/r/imhex/plugins/builtin/source/content/pl_pragmas.cpp",
        "dep/r/imhex/plugins/builtin/source/content/pl_visualizers.cpp",
        "dep/r/imhex/plugins/builtin/source/content/pl_visualizers/chunk_entropy.cpp",
        "dep/r/imhex/plugins/builtin/source/content/pl_visualizers/hex_viewer.cpp",
        "dep/r/imhex/plugins/builtin/source/content/popups/hex_editor/popup_hex_editor_find.cpp",
        "dep/r/imhex/plugins/builtin/source/content/project.cpp",
        "dep/r/imhex/plugins/builtin/source/content/providers/base64_provider.cpp",
        "dep/r/imhex/plugins/builtin/source/content/providers/disk_provider.cpp",
        "dep/r/imhex/plugins/builtin/source/content/providers/file_provider.cpp",
        "dep/r/imhex/plugins/builtin/source/content/providers/gdb_provider.cpp",
        "dep/r/imhex/plugins/builtin/source/content/providers/intel_hex_provider.cpp",
        "dep/r/imhex/plugins/builtin/source/content/providers/memory_file_provider.cpp",
        "dep/r/imhex/plugins/builtin/source/content/providers/motorola_srec_provider.cpp",
        "dep/r/imhex/plugins/builtin/source/content/providers/process_memory_provider.cpp",
        "dep/r/imhex/plugins/builtin/source/content/providers/udp_provider.cpp",
        "dep/r/imhex/plugins/builtin/source/content/providers/view_provider.cpp",
        "dep/r/imhex/plugins/builtin/source/content/recent.cpp",
        "dep/r/imhex/plugins/builtin/source/content/report_generators.cpp",
        "dep/r/imhex/plugins/builtin/source/content/settings_entries.cpp",
        "dep/r/imhex/plugins/builtin/source/content/text_highlighting/pattern_language.cpp",
        "dep/r/imhex/plugins/builtin/source/content/themes.cpp",
        "dep/r/imhex/plugins/builtin/source/content/tools/ascii_table.cpp",
        "dep/r/imhex/plugins/builtin/source/content/tools/base_converter.cpp",
        "dep/r/imhex/plugins/builtin/source/content/tools/byte_swapper.cpp",
        "dep/r/imhex/plugins/builtin/source/content/tools/color_picker.cpp",
        "dep/r/imhex/plugins/builtin/source/content/tools/demangler.cpp",
        "dep/r/imhex/plugins/builtin/source/content/tools/euclidean_alg.cpp",
        "dep/r/imhex/plugins/builtin/source/content/tools/file_tool_combiner.cpp",
        "dep/r/imhex/plugins/builtin/source/content/tools/file_tool_shredder.cpp",
        "dep/r/imhex/plugins/builtin/source/content/tools/file_tool_splitter.cpp",
        "dep/r/imhex/plugins/builtin/source/content/tools/file_uploader.cpp",
        "dep/r/imhex/plugins/builtin/source/content/tools/graphing_calc.cpp",
        "dep/r/imhex/plugins/builtin/source/content/tools/http_requests.cpp",
        "dep/r/imhex/plugins/builtin/source/content/tools/ieee_decoder.cpp",
        "dep/r/imhex/plugins/builtin/source/content/tools/math_eval.cpp",
        "dep/r/imhex/plugins/builtin/source/content/tools/multiplication_decoder.cpp",
        "dep/r/imhex/plugins/builtin/source/content/tools/perms_calc.cpp",
        "dep/r/imhex/plugins/builtin/source/content/tools/regex_replacer.cpp",
        "dep/r/imhex/plugins/builtin/source/content/tools/tcp_client_server.cpp",
        "dep/r/imhex/plugins/builtin/source/content/tools/wiki_explainer.cpp",
        "dep/r/imhex/plugins/builtin/source/content/tools_entries.cpp",
        "dep/r/imhex/plugins/builtin/source/content/views/fullscreen/view_fullscreen_file_info.cpp",
        "dep/r/imhex/plugins/builtin/source/content/views/fullscreen/view_fullscreen_save_editor.cpp",
        "dep/r/imhex/plugins/builtin/source/content/views/view_about.cpp",
        "dep/r/imhex/plugins/builtin/source/content/views/view_bookmarks.cpp",
        "dep/r/imhex/plugins/builtin/source/content/views/view_command_palette.cpp",
        "dep/r/imhex/plugins/builtin/source/content/views/view_constants.cpp",
        "dep/r/imhex/plugins/builtin/source/content/views/view_data_inspector.cpp",
        "dep/r/imhex/plugins/builtin/source/content/views/view_find.cpp",
        "dep/r/imhex/plugins/builtin/source/content/views/view_hex_editor.cpp",
        "dep/r/imhex/plugins/builtin/source/content/views/view_highlight_rules.cpp",
        "dep/r/imhex/plugins/builtin/source/content/views/view_information.cpp",
        "dep/r/imhex/plugins/builtin/source/content/views/view_logs.cpp",
        "dep/r/imhex/plugins/builtin/source/content/views/view_patches.cpp",
        "dep/r/imhex/plugins/builtin/source/content/views/view_pattern_data.cpp",
        "dep/r/imhex/plugins/builtin/source/content/views/view_pattern_editor.cpp",
        "dep/r/imhex/plugins/builtin/source/content/views/view_provider_settings.cpp",
        "dep/r/imhex/plugins/builtin/source/content/views/view_settings.cpp",
        "dep/r/imhex/plugins/builtin/source/content/views/view_store.cpp",
        "dep/r/imhex/plugins/builtin/source/content/views/view_theme_manager.cpp",
        "dep/r/imhex/plugins/builtin/source/content/views/view_tools.cpp",
        "dep/r/imhex/plugins/builtin/source/content/window_decoration.cpp",
        "dep/r/imhex/plugins/builtin/source/content/workspaces.cpp",
        "dep/r/imhex/plugins/builtin/source/plugin_builtin.cpp",
    ],
    hdrs={
        f: f"dep/r/imhex/plugins/builtin/include/{f}"
        for f in [
            "content/export_formatters/export_formatter_csv.hpp",
            "content/export_formatters/export_formatter_tsv.hpp",
            "content/export_formatters/export_formatter_json.hpp",
            "content/export_formatters/export_formatter.hpp",
            "content/popups/hex_editor/popup_hex_editor_find.hpp",
            "content/popups/popup_blocking_task.hpp",
            "content/popups/popup_unsaved_changes.hpp",
            "content/popups/popup_docs_question.hpp",
            "content/popups/popup_tasks_waiting.hpp",
            "content/popups/popup_crash_recovered.hpp",
            "content/text_highlighting/pattern_language.hpp",
            "content/recent.hpp",
            "content/global_actions.hpp",
            "content/providers/base64_provider.hpp",
            "content/providers/gdb_provider.hpp",
            "content/providers/view_provider.hpp",
            "content/providers/process_memory_provider.hpp",
            "content/providers/disk_provider.hpp",
            "content/providers/null_provider.hpp",
            "content/providers/udp_provider.hpp",
            "content/providers/intel_hex_provider.hpp",
            "content/providers/undo_operations/operation_insert.hpp",
            "content/providers/undo_operations/operation_remove.hpp",
            "content/providers/undo_operations/operation_write.hpp",
            "content/providers/undo_operations/operation_bookmark.hpp",
            "content/providers/motorola_srec_provider.hpp",
            "content/providers/memory_file_provider.hpp",
            "content/providers/file_provider.hpp",
            "content/tools_entries.hpp",
            "content/helpers/diagrams.hpp",
            "content/views/view_constants.hpp",
            "content/views/view_theme_manager.hpp",
            "content/views/view_about.hpp",
            "content/views/fullscreen/view_fullscreen_file_info.hpp",
            "content/views/fullscreen/view_fullscreen_save_editor.hpp",
            "content/views/view_patches.hpp",
            "content/views/view_tutorials.hpp",
            "content/views/view_data_inspector.hpp",
            "content/views/view_bookmarks.hpp",
            "content/views/view_hex_editor.hpp",
            "content/views/view_achievements.hpp",
            "content/views/view_tools.hpp",
            "content/views/view_data_processor.hpp",
            "content/views/view_command_palette.hpp",
            "content/views/view_settings.hpp",
            "content/views/view_logs.hpp",
            "content/views/view_highlight_rules.hpp",
            "content/views/view_information.hpp",
            "content/views/view_pattern_editor.hpp",
            "content/views/view_find.hpp",
            "content/views/view_provider_settings.hpp",
            "content/views/view_pattern_data.hpp",
            "content/views/view_store.hpp           ",
            "content/command_line_interface.hpp",
            "content/data_processor_nodes.hpp",
            "content/differing_byte_searcher.hpp",
        ]
    },
    romfsdir="dep/r/imhex/plugins/builtin/romfs",
    deps=[
        ".+fonts-plugin",
        ".+libimhex",
        "dep+patternlanguage_lib",
        ".+libtrace",
        ".+ui-plugin",
        "dep+libwolv_lib",
        "dep/r/imhex/plugins/builtin/source/content/ui_items.cpp",
    ],
)

plugin(
    name="gui-plugin",
    id="gui",
    srcs=[
        "./imhex_overrides/splash_window.cpp",
        "dep/r/imhex/main/gui/include/crash_handlers.hpp",
        "dep/r/imhex/main/gui/include/messaging.hpp",
        "dep/r/imhex/main/gui/include/window.hpp",
        "dep/r/imhex/main/gui/source/crash_handlers.cpp",
        "dep/r/imhex/main/gui/source/init/run/cli.cpp",
        "dep/r/imhex/main/gui/source/init/run/common.cpp",
        "dep/r/imhex/main/gui/source/init/run/desktop.cpp",
        "dep/r/imhex/main/gui/source/init/tasks.cpp",
        "dep/r/imhex/main/gui/source/main.cpp",
        "dep/r/imhex/main/gui/source/messaging/common.cpp",
        "dep/r/imhex/main/gui/source/window/window.cpp",
    ]
    + (
        [
            "dep/r/imhex/main/gui/source/window/platform/macos.cpp",
            "dep/r/imhex/main/gui/source/messaging/macos.cpp",
        ]
        if config.osx
        else (
            [
                "dep/r/imhex/main/gui/source/window/platform/windows.cpp",
                "dep/r/imhex/main/gui/source/messaging/windows.cpp",
            ]
            if config.windows
            else [
                "dep/r/imhex/main/gui/source/window/platform/linux.cpp",
                "dep/r/imhex/main/gui/source/messaging/linux.cpp",
            ]
        )
    ),
    hdrs={
        f: f"dep/r/imhex/main/gui/include/{f}"
        for f in [
            "window.hpp",
            "crash_handlers.hpp",
            "init/tasks.hpp",
            "init/splash_window.hpp",
            "init/run.hpp",
            "messaging.hpp",
        ]
    },
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
        "./exerciserview.cc",
        "./exerciserview.h",
        "./diskprovider.cc",
        "./diskprovider.h",
        "./fluxengine.cc",
        "./globals.h",
        "./imageview.cc",
        "./imageview.h",
        "./logview.cc",
        "./logview.h",
        "./physicalview.cc",
        "./physicalview.h",
        "./summaryview.cc",
        "./summaryview.h",
        "./visualiserview.cc",
        "./visualiserview.h",
        "./utils.cc",
        "./utils.h",
    ],
    hdrs={},
    romfsdir="src/gui2/rsrc",
    deps=[
        "+protocol",
        "+z_lib",
        ".+fonts-plugin",
        ".+libimhex",
        ".+ui-plugin",
        "dep+libusbp_lib",
        "lib/config",
        "lib/core",
        "lib/data",
        "lib/usb",
        "lib/vfs",
        "src/formats",
        "src/gui/drivetypes",
    ],
)

cxxprogram(
    name="gui2",
    srcs=[
        "./main.cc",
    ],
    cflags=cflags,
    ldflags=["-lmbedcrypto"]
    + ([] if config.osx or config.windows else ["-lboost_regex"])
    + (["-ldl"] if config.unix else [])
    + (["-ldwmapi", "-lnetapi32"] if config.windows else []),
    deps=[
        "dep+patternlanguage_lib",
        "dep+fmt_lib",
        ".+builtin-plugin",
        ".+fonts-plugin",
        ".+ui-plugin",
        ".+gui-plugin",
        ".+fluxengine-plugin",
    ]
    # Windows needs this, for some reason.
    + (config.windows and [package(name="tre_lib", package="tre")] or []),
)

if config.osx:
    simplerule(
        name="fluxengine_app_zip",
        ins=[
            ".+gui2",
            "extras+fluxengine_icns",
            "extras+fluxengine_template",
        ],
        outs=["=FluxEngine.app.zip"],
        commands=[
            "rm -rf $[dir]/FluxEngine.app",
            "unzip -q -d $[dir] $[ins[2]]",  # creates FluxEngine.app
            "cp $[ins[0]] $[dir]/FluxEngine.app/Contents/MacOS/fluxengine-gui",
            "mkdir -p $[dir]/FluxEngine.app/Contents/Resources",
            "cp $[ins[1]] $[dir]/FluxEngine.app/Contents/Resources/FluxEngine.icns",
            "dylibbundler -of -x $[dir]/FluxEngine.app/Contents/MacOS/fluxengine-gui -b -d $[dir]/FluxEngine.app/Contents/libs -cd > /dev/null",
            # "cp $$(brew --prefix wxwidgets)/README.md FluxEngine.app/Contents/libs/wxWidgets.md",
            # "cp $$(brew --prefix protobuf)/LICENSE FluxEngine.app/Contents/libs/protobuf.txt",
            # "cp $$(brew --prefix fmt)/LICENSE* FluxEngine.app/Contents/libs/fmt.rst",
            # "cp $$(brew --prefix libpng)/LICENSE FluxEngine.app/Contents/libs/libpng.txt",
            # "cp $$(brew --prefix libjpeg)/README FluxEngine.app/Contents/libs/libjpeg.txt",
            # "cp $$(brew --prefix abseil)/LICENSE FluxEngine.app/Contents/libs/abseil.txt",
            # "cp $$(brew --prefix libtiff)/LICENSE.md FluxEngine.app/Contents/libs/libtiff.txt",
            # "cp $$(brew --prefix zstd)/LICENSE FluxEngine.app/Contents/libs/zstd.txt",
            "(cd $[dir] && zip -rq FluxEngine.app.zip FluxEngine.app)",
            "mv $[dir]/FluxEngine.app.zip $[outs[0]]",
        ],
        label="MKAPP",
    )

    simplerule(
        name="fluxengine_pkg",
        ins=[
            ".+fluxengine_app_zip",
        ],
        outs=["=FluxEngine.pkg"],
        commands=[
            "rm -rf $[dir]/FluxEngine.app",
            "unzip -q -d $[dir] $[ins[0]]",
            "pkgbuild --quiet --install-location /Applications --component $[dir]/FluxEngine.app $[outs[0]]",
        ],
        label="MKPKG",
    )
