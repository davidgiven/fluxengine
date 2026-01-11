from build.pkg import package
from build.c import clibrary, cxxlibrary
from build.git import git_repository
import config

package(
    name="fmt_lib",
    package="fmt",
    fallback=cxxlibrary(
        name="fmt_fallback_lib",
        srcs=[
            "dep/r/fmt/src/format.cc",
            "dep/r/fmt/src/os.cc",
        ],
        hdrs={
            h: f"dep/r/fmt/include/{h}"
            for h in [
                "fmt/args.h",
                "fmt/base.h",
                "fmt/chrono.h",
                "fmt/color.h",
                "fmt/compile.h",
                "fmt/core.h",
                "fmt/format.h",
                "fmt/format-inl.h",
                "fmt/os.h",
                "fmt/ostream.h",
                "fmt/printf.h",
                "fmt/ranges.h",
                "fmt/std.h",
                "fmt/xchar.h",
            ]
        },
        deps=[
            git_repository(
                name="fmt_repo",
                url="https://github.com/fmtlib/fmt",
                branch="11.1.4",
                path="dep/r/fmt",
            )
        ],
    ),
)

package(
    name="md4c_lib",
    package="md4c",
    fallback=clibrary(
        name="md4c_fallback_lib",
        srcs=[
            "dep/r/md4c/src/entity.c",
            "dep/r/md4c/src/entity.h",
            "dep/r/md4c/src/md4c-html.h",
            "dep/r/md4c/src/md4c.c",
            "dep/r/md4c/src/md4c.h",
        ],
        hdrs={"md4c.h": "dep/r/md4c/src/md4c.h"},
        deps=[
            git_repository(
                name="md4c_repo",
                url="https://github.com/mity/md4c",
                branch="master",
                path="dep/r/md4c",
            )
        ],
    ),
)

cxxlibrary(
    name="lexy_lib",
    hdrs={
        h: f"dep/r/lexy/include/{h}"
        for h in [
            "lexy_ext/parse_tree_algorithm.hpp",
            "lexy_ext/report_error.hpp",
            "lexy_ext/parse_tree_doctest.hpp",
            "lexy_ext/compiler_explorer.hpp",
            "lexy_ext/shell.hpp",
            "lexy/callback/constant.hpp",
            "lexy/callback/object.hpp",
            "lexy/callback/composition.hpp",
            "lexy/callback/aggregate.hpp",
            "lexy/callback/forward.hpp",
            "lexy/callback/integer.hpp",
            "lexy/callback/string.hpp",
            "lexy/callback/bit_cast.hpp",
            "lexy/callback/bind.hpp",
            "lexy/callback/fold.hpp",
            "lexy/callback/adapter.hpp",
            "lexy/callback/container.hpp",
            "lexy/callback/noop.hpp",
            "lexy/callback/base.hpp",
            "lexy/error.hpp",
            "lexy/_detail/type_name.hpp",
            "lexy/_detail/assert.hpp",
            "lexy/_detail/swar.hpp",
            "lexy/_detail/tuple.hpp",
            "lexy/_detail/code_point.hpp",
            "lexy/_detail/unicode_database.hpp",
            "lexy/_detail/integer_sequence.hpp",
            "lexy/_detail/any_ref.hpp",
            "lexy/_detail/memory_resource.hpp",
            "lexy/_detail/iterator.hpp",
            "lexy/_detail/config.hpp",
            "lexy/_detail/stateless_lambda.hpp",
            "lexy/_detail/lazy_init.hpp",
            "lexy/_detail/nttp_string.hpp",
            "lexy/_detail/detect.hpp",
            "lexy/_detail/string_view.hpp",
            "lexy/_detail/buffer_builder.hpp",
            "lexy/_detail/invoke.hpp",
            "lexy/_detail/std.hpp",
            "lexy/code_point.hpp",
            "lexy/parse_tree.hpp",
            "lexy/grammar.hpp",
            "lexy/encoding.hpp",
            "lexy/input_location.hpp",
            "lexy/lexeme.hpp",
            "lexy/dsl/capture.hpp",
            "lexy/dsl/case_folding.hpp",
            "lexy/dsl/literal.hpp",
            "lexy/dsl/context_identifier.hpp",
            "lexy/dsl/error.hpp",
            "lexy/dsl/whitespace.hpp",
            "lexy/dsl/flags.hpp",
            "lexy/dsl/subgrammar.hpp",
            "lexy/dsl/code_point.hpp",
            "lexy/dsl/sign.hpp",
            "lexy/dsl/member.hpp",
            "lexy/dsl/if.hpp",
            "lexy/dsl/symbol.hpp",
            "lexy/dsl/branch.hpp",
            "lexy/dsl/follow.hpp",
            "lexy/dsl/repeat.hpp",
            "lexy/dsl/ascii.hpp",
            "lexy/dsl/combination.hpp",
            "lexy/dsl/return.hpp",
            "lexy/dsl/integer.hpp",
            "lexy/dsl/production.hpp",
            "lexy/dsl/until.hpp",
            "lexy/dsl/brackets.hpp",
            "lexy/dsl/sequence.hpp",
            "lexy/dsl/char_class.hpp",
            "lexy/dsl/lookahead.hpp",
            "lexy/dsl/loop.hpp",
            "lexy/dsl/unicode.hpp",
            "lexy/dsl/context_counter.hpp",
            "lexy/dsl/choice.hpp",
            "lexy/dsl/parse_as.hpp",
            "lexy/dsl/byte.hpp",
            "lexy/dsl/digit.hpp",
            "lexy/dsl/punctuator.hpp",
            "lexy/dsl/recover.hpp",
            "lexy/dsl/times.hpp",
            "lexy/dsl/bits.hpp",
            "lexy/dsl/option.hpp",
            "lexy/dsl/identifier.hpp",
            "lexy/dsl/context_flag.hpp",
            "lexy/dsl/operator.hpp",
            "lexy/dsl/token.hpp",
            "lexy/dsl/any.hpp",
            "lexy/dsl/separator.hpp",
            "lexy/dsl/peek.hpp",
            "lexy/dsl/newline.hpp",
            "lexy/dsl/parse_tree_node.hpp",
            "lexy/dsl/expression.hpp",
            "lexy/dsl/delimited.hpp",
            "lexy/dsl/effect.hpp",
            "lexy/dsl/position.hpp",
            "lexy/dsl/scan.hpp",
            "lexy/dsl/list.hpp",
            "lexy/dsl/base.hpp",
            "lexy/dsl/bom.hpp",
            "lexy/dsl/terminator.hpp",
            "lexy/dsl/eof.hpp",
            "lexy/dsl.hpp",
            "lexy/visualize.hpp",
            "lexy/callback.hpp",
            "lexy/token.hpp",
            "lexy/action/parse_as_tree.hpp",
            "lexy/action/parse.hpp",
            "lexy/action/trace.hpp",
            "lexy/action/match.hpp",
            "lexy/action/validate.hpp",
            "lexy/action/scan.hpp",
            "lexy/action/base.hpp",
            "lexy/input/buffer.hpp",
            "lexy/input/lexeme_input.hpp",
            "lexy/input/argv_input.hpp",
            "lexy/input/file.hpp",
            "lexy/input/string_input.hpp",
            "lexy/input/parse_tree_input.hpp",
            "lexy/input/range_input.hpp",
            "lexy/input/base.hpp",
        ]
    },
    deps=[
        git_repository(
            name="lexy_repo",
            url="https://github.com/foonathan/lexy",
            branch="v2025.05.0",
            path="dep/r/lexy",
        )
    ],
)

cxxlibrary(
    name="snowhouse_lib",
    hdrs={
        h: f"dep/r/snowhouse/include/{h}"
        for h in [
            "snowhouse/snowhouse.h",
            "snowhouse/assert.h",
            "snowhouse/fluent/fluent.h",
            "snowhouse/fluent/constraintadapter.h",
            "snowhouse/fluent/constraintlist.h",
            "snowhouse/fluent/operators/andoperator.h",
            "snowhouse/fluent/operators/invalidexpressionexception.h",
            "snowhouse/fluent/operators/collections/collectionoperator.h",
            "snowhouse/fluent/operators/collections/collectionconstraintevaluator.h",
            "snowhouse/fluent/operators/collections/atleastoperator.h",
            "snowhouse/fluent/operators/collections/noneoperator.h",
            "snowhouse/fluent/operators/collections/atmostoperator.h",
            "snowhouse/fluent/operators/collections/alloperator.h",
            "snowhouse/fluent/operators/collections/exactlyoperator.h",
            "snowhouse/fluent/operators/notoperator.h",
            "snowhouse/fluent/operators/constraintoperator.h",
            "snowhouse/fluent/operators/oroperator.h",
            "snowhouse/fluent/expressionbuilder.h",
            "snowhouse/assertionexception.h",
            "snowhouse/exceptions.h",
            "snowhouse/stringizers.h",
            "snowhouse/macros.h",
            "snowhouse/constraints/equalscontainerconstraint.h",
            "snowhouse/constraints/islessthanorequaltoconstraint.h",
            "snowhouse/constraints/equalsconstraint.h",
            "snowhouse/constraints/isgreaterthanconstraint.h",
            "snowhouse/constraints/fulfillsconstraint.h",
            "snowhouse/constraints/endswithconstraint.h",
            "snowhouse/constraints/constraints.h",
            "snowhouse/constraints/haslengthconstraint.h",
            "snowhouse/constraints/startswithconstraint.h",
            "snowhouse/constraints/equalswithdeltaconstraint.h",
            "snowhouse/constraints/isgreaterthanorequaltoconstraint.h",
            "snowhouse/constraints/containsconstraint.h",
            "snowhouse/constraints/islessthanconstraint.h",
            "snowhouse/constraints/isemptyconstraint.h",
            "snowhouse/constraints/expressions/andexpression.h",
            "snowhouse/constraints/expressions/orexpression.h",
            "snowhouse/constraints/expressions/expression_fwd.h",
            "snowhouse/constraints/expressions/notexpression.h",
            "snowhouse/constraints/expressions/expression.h",
            "snowhouse/stringize.h",
        ]
    },
    deps=[
        git_repository(
            name="snowhouse_repo",
            url="https://github.com/banditcpp/snowhouse",
            branch="v5.0.0",
            path="dep/r/snowhouse",
        )
    ],
)

package(
    name="cli11_lib",
    package="CLI11",
    fallback=cxxlibrary(
        name="cli11_fallback_lib",
        srcs=[],
        hdrs={
            h: f"dep/r/cli11/include/{h}"
            for h in [
                "CLI/Error.hpp",
                "CLI/App.hpp",
                "CLI/ExtraValidators.hpp",
                "CLI/ConfigFwd.hpp",
                "CLI/impl/Encoding_inl.hpp",
                "CLI/impl/Config_inl.hpp",
                "CLI/impl/Split_inl.hpp",
                "CLI/impl/Validators_inl.hpp",
                "CLI/impl/ExtraValidators_inl.hpp",
                "CLI/impl/App_inl.hpp",
                "CLI/impl/Option_inl.hpp",
                "CLI/impl/Formatter_inl.hpp",
                "CLI/impl/StringTools_inl.hpp",
                "CLI/impl/Argv_inl.hpp",
                "CLI/Split.hpp",
                "CLI/Version.hpp",
                "CLI/Config.hpp",
                "CLI/StringTools.hpp",
                "CLI/TypeTools.hpp",
                "CLI/FormatterFwd.hpp",
                "CLI/CLI.hpp",
                "CLI/Macros.hpp",
                "CLI/Formatter.hpp",
                "CLI/Option.hpp",
                "CLI/Argv.hpp",
                "CLI/Encoding.hpp",
                "CLI/Validators.hpp",
                "CLI/Timer.hpp",
            ]
        },
        deps=[
            git_repository(
                name="cli11_repo",
                url="https://github.com/CLIUtils/CLI11",
                branch="v2.6.1",
                path="dep/r/cli11",
            )
        ],
    ),
)

package(
    name="nlohmannjson_lib",
    package="nlohmann_json",
    fallback=cxxlibrary(
        name="nlohmannjson_fallback_lib",
        srcs=[],
        hdrs={
            h: f"dep/r/nlohmann_json/single_include/{h}"
            for h in ["nlohmann/json_fwd.hpp", "nlohmann/json.hpp"]
        },
        deps=[
            git_repository(
                name="nlohmannjson_repo",
                url="https://github.com/nlohmann/json",
                branch="v3.12.0",
                path="dep/r/nlohmann_json",
            )
        ],
    ),
)

cxxlibrary(
    name="libthrowingptr",
    srcs=[],
    hdrs={
        h: f"dep/r/throwing_ptr/include/{h}"
        for h in [
            "throwing/unique_ptr.hpp",
            "throwing/private/clear_compiler_checks.hpp",
            "throwing/private/compiler_checks.hpp",
            "throwing/shared_ptr.hpp",
            "throwing/null_ptr_exception.hpp",
        ]
    },
    deps=[
        git_repository(
            name="libthrowingptr_repo",
            url="https://github.com/rockdreamer/throwing_ptr",
            branch="master",
            path="dep/r/throwing_ptr",
        )
    ],
)

cxxlibrary(
    name="xdgpp_lib",
    srcs=[],
    hdrs={"xdg.hpp": "dep/r/xdgpp/xdg.hpp"},
    deps=[
        git_repository(
            name="xdgpp_repo",
            url="https://github.com/WerWolv/xdgpp",
            branch="master",
            path="dep/r/xdgpp",
        )
    ],
)

package(
    name="plutovg_lib",
    package="plutovg",
    fallback=clibrary(
        name="plutovg_fallback_lib",
        srcs=[
            "dep/r/plutovg/source/plutovg-canvas.c",
            "dep/r/plutovg/source/plutovg-ft-math.c",
            "dep/r/plutovg/source/plutovg-surface.c",
            "dep/r/plutovg/source/plutovg-font.c",
            "dep/r/plutovg/source/plutovg-ft-math.h",
            "dep/r/plutovg/source/plutovg-ft-stroker.h",
            "dep/r/plutovg/source/plutovg-rasterize.c",
            "dep/r/plutovg/source/plutovg-stb-image-write.h",
            "dep/r/plutovg/source/plutovg-blend.c",
            "dep/r/plutovg/source/plutovg-ft-stroker.c",
            "dep/r/plutovg/source/plutovg-private.h",
            "dep/r/plutovg/source/plutovg-ft-raster.h",
            "dep/r/plutovg/source/plutovg-ft-types.h",
            "dep/r/plutovg/source/plutovg-stb-image.h",
            "dep/r/plutovg/source/plutovg-stb-truetype.h",
            "dep/r/plutovg/source/plutovg-paint.c",
            "dep/r/plutovg/source/plutovg-matrix.c",
            "dep/r/plutovg/source/plutovg-ft-raster.c",
            "dep/r/plutovg/source/plutovg-utils.h",
            "dep/r/plutovg/source/plutovg-path.c",
        ],
        hdrs={"plutovg.h": "dep/r/plutovg/include/plutovg.h"},
        deps=[
            git_repository(
                name="plutovg_repo",
                url="https://github.com/sammycage/plutovg",
                branch="v1.3.2",
                path="dep/r/plutovg",
            )
        ],
        cflags=[
            "-DPLUTOVG_BUILD_STATIC",
        ],
        caller_cflags=[
            "-DPLUTOVG_BUILD_STATIC",
        ],
    ),
)

package(
    name="lunasvg",
    package="lunasvg",
    fallback=cxxlibrary(
        name="lunasvg_fallback_lib",
        srcs=[
            "dep/r/lunasvg/source/svgtextelement.cpp",
            "dep/r/lunasvg/source/lunasvg.cpp",
            "dep/r/lunasvg/source/graphics.h",
            "dep/r/lunasvg/source/svggeometryelement.cpp",
            "dep/r/lunasvg/source/svgproperty.cpp",
            "dep/r/lunasvg/source/graphics.cpp",
            "dep/r/lunasvg/source/svgpaintelement.cpp",
            "dep/r/lunasvg/source/svgparser.cpp",
            "dep/r/lunasvg/source/svgelement.cpp",
            "dep/r/lunasvg/source/svgrenderstate.h",
            "dep/r/lunasvg/source/svgproperty.h",
            "dep/r/lunasvg/source/svgpaintelement.h",
            "dep/r/lunasvg/source/svgelement.h",
            "dep/r/lunasvg/source/svgtextelement.h",
            "dep/r/lunasvg/source/svggeometryelement.h",
            "dep/r/lunasvg/source/svgparserutils.h",
            "dep/r/lunasvg/source/svglayoutstate.cpp",
            "dep/r/lunasvg/source/svglayoutstate.h",
            "dep/r/lunasvg/source/svgrenderstate.cpp",
        ],
        hdrs={"lunasvg.h": "dep/r/lunasvg/include/lunasvg.h"},
        deps=[
            "dep+plutovg_lib",
            "dep+fmt_lib",
            git_repository(
                name="lunasvg_repo",
                url="https://github.com/sammycage/lunasvg",
                branch="v3.5.0",
                path="dep/r/lunasvg",
            ),
        ],
        cflags=[
            "-DLUNASVG_BUILD_STATIC",
        ],
        caller_cflags=[
            "-DLUNASVG_BUILD_STATIC",
        ],
    ),
)

git_repository(
    name="nativefiledialog_repo",
    url="https://github.com/btzy/nativefiledialog-extended",
    branch="v1.3.0",
    path="dep/r/native-file-dialog",
)

if config.osx:
    clibrary(
        name="nativefiledialog_lib",
        srcs=["dep/r/native-file-dialog/src/nfd_cocoa.m"],
        hdrs={
            "nfd.hpp": "dep/r/native-file-dialog/src/include/nfd.hpp",
            "nfd.h": "dep/r/native-file-dialog/src/include/nfd.h",
        },
        deps=[".+nativefiledialog_repo"],
    )
elif config.windows:
    cxxlibrary(
        name="nativefiledialog_lib",
        srcs=(["dep/r/native-file-dialog/src/nfd_win.cpp"]),
        hdrs={
            "nfd.hpp": "dep/r/native-file-dialog/src/include/nfd.hpp",
            "nfd.h": "dep/r/native-file-dialog/src/include/nfd.h",
        },
        deps=[".+nativefiledialog_repo"],
    )
else:
    package(name="dbus_lib", package="dbus-1")
    cxxlibrary(
        name="nativefiledialog_lib",
        srcs=(["dep/r/native-file-dialog/src/nfd_portal.cpp"]),
        hdrs={
            "nfd.hpp": "dep/r/native-file-dialog/src/include/nfd.hpp",
            "nfd.h": "dep/r/native-file-dialog/src/include/nfd.h",
        },
        deps=[".+dbus_lib", ".+nativefiledialog_repo"],
    )

clibrary(
    name="libusbp_lib",
    srcs=[
        "dep/r/libusbp/src/async_in_pipe.c",
        "dep/r/libusbp/src/error.c",
        "dep/r/libusbp/src/error_hresult.c",
        "dep/r/libusbp/src/find_device.c",
        "dep/r/libusbp/src/list.c",
        "dep/r/libusbp/src/pipe_id.c",
        "dep/r/libusbp/src/string.c",
        "dep/r/libusbp/src/libusbp_internal.h",
        "dep/r/libusbp/include/libusbp_config.h",
        "dep/r/libusbp/include/libusbp.h",
    ]
    + (
        [
            "dep/r/libusbp/src/windows/async_in_transfer_windows.c",
            "dep/r/libusbp/src/windows/device_instance_id_windows.c",
            "dep/r/libusbp/src/windows/device_windows.c",
            "dep/r/libusbp/src/windows/error_windows.c",
            "dep/r/libusbp/src/windows/generic_handle_windows.c",
            "dep/r/libusbp/src/windows/generic_interface_windows.c",
            "dep/r/libusbp/src/windows/interface_windows.c",
            "dep/r/libusbp/src/windows/list_windows.c",
            "dep/r/libusbp/src/windows/serial_port_windows.c",
        ]
        if config.windows
        else (
            [
                "dep/r/libusbp/src/mac/async_in_transfer_mac.c",
                "dep/r/libusbp/src/mac/device_mac.c",
                "dep/r/libusbp/src/mac/error_mac.c",
                "dep/r/libusbp/src/mac/generic_handle_mac.c",
                "dep/r/libusbp/src/mac/generic_interface_mac.c",
                "dep/r/libusbp/src/mac/iokit_mac.c",
                "dep/r/libusbp/src/mac/list_mac.c",
                "dep/r/libusbp/src/mac/serial_port_mac.c",
            ]
            if config.osx
            else [
                "dep/r/libusbp/src/linux/async_in_transfer_linux.c",
                "dep/r/libusbp/src/linux/device_linux.c",
                "dep/r/libusbp/src/linux/error_linux.c",
                "dep/r/libusbp/src/linux/generic_handle_linux.c",
                "dep/r/libusbp/src/linux/generic_interface_linux.c",
                "dep/r/libusbp/src/linux/list_linux.c",
                "dep/r/libusbp/src/linux/serial_port_linux.c",
                "dep/r/libusbp/src/linux/udev_linux.c",
                "dep/r/libusbp/src/linux/usbfd_linux.c",
            ]
        )
    ),
    cflags=[
        "-Wno-deprecated-declarations",
    ],
    caller_ldflags=(
        ["-lsetupapi", "-lwinusb", "-lole32", "-luuid"]
        if config.windows
        else []
    ),
    deps=(
        [package(name="udev_lib", package="libudev")]
        if not config.windows and not config.osx
        else []
    )
    + [
        git_repository(
            name="libusbp_repo",
            url="https://github.com/pololu/libusbp",
            branch="master",
            path="dep/r/libusbp",
        )
    ],
    hdrs={
        "libusbp_internal.h": "dep/r/libusbp/src/libusbp_internal.h",
        "libusbp_config.h": "dep/libusbp_config.h",
        "libusbp.hpp": "dep/r/libusbp/include/libusbp.hpp",
        "libusbp.h": "dep/r/libusbp/include/libusbp.h",
    },
)

clibrary(
    name="stb_lib",
    srcs=["dep/stb_image_write.c"],
    hdrs={"stb_image_write.h": "dep/r/stb/stb_image_write.h"},
    deps=[
        git_repository(
            name="stb_repo",
            url="https://github.com/nothings/stb",
            branch="master",
            path="dep/r/stb",
        )
    ],
)

# All this is libwolv.

git_repository(
    name="libwolv_repo",
    url="https://github.com/WerWolv/libwolv",
    branch="master",
    path="dep/r/libwolv",
)

libwolv_cflags = (
    ["-DOS_MACOS"]
    if config.osx
    else ["-DOS_WINDOWS"] if config.windows else ["-DOS_LINUX"]
)

cxxlibrary(
    name="libwolv_core",
    srcs=(
        [
            "dep/r/libwolv/libs/io/source/io/file.cpp",
            "dep/r/libwolv/libs/io/source/io/fs.cpp",
            "dep/r/libwolv/libs/io/source/io/handle.cpp",
            "dep/r/libwolv/libs/math_eval/source/math_eval/math_evaluator.cpp",
            "dep/r/libwolv/libs/net/source/net/common.cpp",
            "dep/r/libwolv/libs/net/source/net/socket_client.cpp",
            "dep/r/libwolv/libs/net/source/net/socket_server.cpp",
            "dep/r/libwolv/libs/utils/source/utils/string.cpp",
        ]
        + (
            ["dep/r/libwolv/libs/io/source/io/file_unix.cpp"]
            if not config.windows
            else []
        )
    ),
    hdrs=(
        {"jthread.hpp": "./jthread.hpp"}
        | {
            f: f"dep/r/libwolv/libs/types/include/{f}"
            for f in [
                "wolv/literals.hpp",
                "wolv/concepts.hpp",
                "wolv/types.hpp",
                "wolv/types/uintwide_t.h",
                "wolv/types/type_name.hpp",
                "wolv/types/static_string.hpp",
            ]
        }
        | {
            f: f"dep/r/libwolv/libs/utils/include/{f}"
            for f in [
                "wolv/utils/charconv.hpp",
                "wolv/utils/expected.hpp",
                "wolv/utils/thread_pool.hpp",
                "wolv/utils/string.hpp",
                "wolv/utils/preproc.hpp",
                "wolv/utils/lock.hpp",
                "wolv/utils/guards.hpp",
                "wolv/utils/core.hpp",
            ]
        }
        | {
            f: f"dep/r/libwolv/libs/math_eval/include/{f}"
            for f in ["wolv/math_eval/math_evaluator.hpp"]
        }
        | {
            f: f"dep/r/libwolv/libs/containers/include/{f}"
            for f in [
                "wolv/container/lazy.hpp",
                "wolv/container/ring_buffer.hpp",
                "wolv/container/interval_tree.hpp",
            ]
        }
        | {
            f: f"dep/r/libwolv/libs/io/include/{f}"
            for f in [
                "wolv/io/buffered_reader.hpp",
                "wolv/io/file.hpp",
                "wolv/io/fs.hpp",
                "wolv/io/fs_macos.hpp",
                "wolv/io/handle.hpp",
            ]
        }
        | {
            f: f"dep/r/libwolv/libs/hash/include/{f}"
            for f in [
                "wolv/hash/crc.hpp",
                "wolv/hash/uuid.hpp",
            ]
        }
        | {
            f: f"dep/r/libwolv/libs/net/include/{f}"
            for f in [
                "wolv/net/common.hpp",
                "wolv/net/socket_client.hpp",
                "wolv/net/socket_server.hpp",
            ]
        }
    ),
    deps=[".+libwolv_repo"],
    cflags=libwolv_cflags,
    caller_cflags=libwolv_cflags,
)

if config.osx:
    clibrary(
        name="libwolv_lib",
        srcs=[
            "dep/r/libwolv/libs/io/source/io/fs_macos.m",
        ],
        deps=[".+libwolv_core"],
    )
elif config.windows:
    cxxlibrary(
        name="libwolv_lib",
        srcs=["dep/r/libwolv/libs/io/source/io/file_win.cpp"],
        deps=[".+libwolv_core"],
    )
else:
    cxxlibrary(
        name="libwolv_lib",
        srcs=[],
        deps=[".+libwolv_core"],
    )

cxxlibrary(
    name="patternlanguage_lib",
    srcs=[
        "dep/r/pattern-language/lib/source/pl/lib/std/math.cpp",
        "dep/r/pattern-language/lib/source/pl/lib/std/core.cpp",
        "dep/r/pattern-language/lib/source/pl/lib/std/time.cpp",
        "dep/r/pattern-language/lib/source/pl/lib/std/string.cpp",
        "dep/r/pattern-language/lib/source/pl/lib/std/std.cpp",
        "dep/r/pattern-language/lib/source/pl/lib/std/pragmas.cpp",
        "dep/r/pattern-language/lib/source/pl/lib/std/random.cpp",
        "dep/r/pattern-language/lib/source/pl/lib/std/file.cpp",
        "dep/r/pattern-language/lib/source/pl/lib/std/mem.cpp",
        "dep/r/pattern-language/lib/source/pl/lib/std/hash.cpp",
        "dep/r/pattern-language/lib/source/pl/pattern_language.cpp",
        "dep/r/pattern-language/lib/source/pl/core/resolvers.cpp",
        "dep/r/pattern-language/lib/source/pl/core/token.cpp",
        "dep/r/pattern-language/lib/source/pl/core/parser.cpp",
        "dep/r/pattern-language/lib/source/pl/core/validator.cpp",
        "dep/r/pattern-language/lib/source/pl/core/resolver.cpp",
        "dep/r/pattern-language/lib/source/pl/core/preprocessor.cpp",
        "dep/r/pattern-language/lib/source/pl/core/evaluator.cpp",
        "dep/r/pattern-language/lib/source/pl/core/lexer.cpp",
        "dep/r/pattern-language/lib/source/pl/core/error.cpp",
        "dep/r/pattern-language/lib/source/pl/core/parser_manager.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_array_variable_decl.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_attribute.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_bitfield.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_bitfield_array_variable_decl.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_bitfield_field.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_builtin_type.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_cast.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_compound_statement.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_conditional_statement.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_control_flow_statement.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_enum.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_function_call.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_function_definition.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_imported_type.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_literal.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_lvalue_assignment.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_match_statement.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_mathematical_expression.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_multi_variable_decl.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_parameter_pack.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_pointer_variable_decl.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_rvalue.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_rvalue_assignment.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_scope_resolution.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_struct.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_ternary_expression.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_try_catch_statement.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_type_decl.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_type_operator.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_union.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_variable_decl.cpp",
        "dep/r/pattern-language/lib/source/pl/core/ast/ast_node_while_statement.cpp",
        "dep/r/pattern-language/lib/source/pl/helpers/utils.cpp",
        "dep/r/pattern-language/cli/source/subcommands/docs.cpp",
        "dep/r/pattern-language/cli/source/subcommands/run.cpp",
        "dep/r/pattern-language/cli/source/subcommands/info.cpp",
        "dep/r/pattern-language/cli/source/subcommands/format.cpp",
        "dep/r/pattern-language/cli/source/main.cpp",
        "dep/r/pattern-language/cli/source/helpers/utils.cpp",
        "dep/r/pattern-language/cli/source/helpers/info_utils.cpp",
    ],
    hdrs=(
        {
            f: f"dep/r/pattern-language/lib/include/{f}"
            for f in [
                "pl/patterns/pattern_enum.hpp",
                "pl/patterns/pattern_union.hpp",
                "pl/patterns/pattern_wide_character.hpp",
                "pl/patterns/pattern_unsigned.hpp",
                "pl/patterns/pattern_bitfield.hpp",
                "pl/patterns/pattern_signed.hpp",
                "pl/patterns/pattern_character.hpp",
                "pl/patterns/pattern_array_dynamic.hpp",
                "pl/patterns/pattern_padding.hpp",
                "pl/patterns/pattern_struct.hpp",
                "pl/patterns/pattern_error.hpp",
                "pl/patterns/pattern.hpp",
                "pl/patterns/pattern_float.hpp",
                "pl/patterns/pattern_string.hpp",
                "pl/patterns/pattern_wide_string.hpp",
                "pl/patterns/pattern_boolean.hpp",
                "pl/patterns/pattern_array_static.hpp",
                "pl/patterns/pattern_pointer.hpp",
                "pl/pattern_visitor.hpp",
                "pl/lib/std/types.hpp",
                "pl/lib/std/libstd.hpp",
                "pl/pattern_language.hpp",
                "pl/core/log_console.hpp",
                "pl/core/preprocessor.hpp",
                "pl/core/resolvers.hpp",
                "pl/core/location.hpp",
                "pl/core/resolver.hpp",
                "pl/core/parser.hpp",
                "pl/core/parser_manager.hpp",
                "pl/core/evaluator.hpp",
                "pl/core/validator.hpp",
                "pl/core/token.hpp",
                "pl/core/ast/ast_node_bitfield.hpp",
                "pl/core/ast/ast_node.hpp",
                "pl/core/ast/ast_node_array_variable_decl.hpp",
                "pl/core/ast/ast_node_attribute.hpp",
                "pl/core/ast/ast_node_bitfield_array_variable_decl.hpp",
                "pl/core/ast/ast_node_bitfield_field.hpp",
                "pl/core/ast/ast_node_builtin_type.hpp",
                "pl/core/ast/ast_node_cast.hpp",
                "pl/core/ast/ast_node_compound_statement.hpp",
                "pl/core/ast/ast_node_conditional_statement.hpp",
                "pl/core/ast/ast_node_control_flow_statement.hpp",
                "pl/core/ast/ast_node_enum.hpp",
                "pl/core/ast/ast_node_function_call.hpp",
                "pl/core/ast/ast_node_function_definition.hpp",
                "pl/core/ast/ast_node_imported_type.hpp",
                "pl/core/ast/ast_node_literal.hpp",
                "pl/core/ast/ast_node_lvalue_assignment.hpp",
                "pl/core/ast/ast_node_match_statement.hpp",
                "pl/core/ast/ast_node_mathematical_expression.hpp",
                "pl/core/ast/ast_node_multi_variable_decl.hpp",
                "pl/core/ast/ast_node_parameter_pack.hpp",
                "pl/core/ast/ast_node_pointer_variable_decl.hpp",
                "pl/core/ast/ast_node_rvalue.hpp",
                "pl/core/ast/ast_node_rvalue_assignment.hpp",
                "pl/core/ast/ast_node_scope_resolution.hpp",
                "pl/core/ast/ast_node_struct.hpp",
                "pl/core/ast/ast_node_ternary_expression.hpp",
                "pl/core/ast/ast_node_try_catch_statement.hpp",
                "pl/core/ast/ast_node_type_decl.hpp",
                "pl/core/ast/ast_node_type_operator.hpp",
                "pl/core/ast/ast_node_union.hpp",
                "pl/core/ast/ast_node_variable_decl.hpp",
                "pl/core/ast/ast_node_while_statement.hpp",
                "pl/core/errors/error.hpp",
                "pl/core/errors/runtime_errors.hpp",
                "pl/core/errors/result.hpp",
                "pl/core/tokens.hpp",
                "pl/core/lexer.hpp",
                "pl/api.hpp",
                "pl/helpers/concepts.hpp",
                "pl/helpers/types.hpp",
                "pl/helpers/safe_pointer.hpp",
                "pl/helpers/buffered_reader.hpp",
                "pl/helpers/safe_iterator.hpp",
                "pl/helpers/utils.hpp",
                "pl/helpers/variant_type_index.hpp",
                "pl.hpp",
            ]
        }
        | {
            f: f"dep/r/pattern-language/generators/include/{f}"
            for f in [
                "pl/formatters/formatter_json.hpp",
                "pl/formatters/formatter_yaml.hpp",
                "pl/formatters/formatter.hpp",
                "pl/formatters/formatter_html.hpp",
                "pl/formatters.hpp",
            ]
        }
        | {
            f: f"dep/r/pattern-language/cli/include/{f}"
            for f in [
                "pl/cli/helpers/info_utils.hpp",
                "pl/cli/helpers/utils.hpp",
                "pl/cli/cli.hpp",
            ]
        }
    ),
    deps=[
        "dep+libthrowingptr",
        "dep+libwolv_lib",
        "dep+fmt_lib",
        "dep+cli11_lib",
        "dep+nlohmannjson_lib",
        git_repository(
            name="patternlanguage_repo",
            url="https://github.com/WerWolv/PatternLanguage",
            branch="ImHex-v1.38.0",
            commit="f97999d4da8f64df0706227f8b5a6a861e5a95ff",
            path="dep/r/pattern-language",
        ),
    ],
)

clibrary(
    name="microtar_lib",
    srcs=["dep/r/microtar/src/microtar.c"],
    hdrs={"microtar.h": "dep/r/microtar/src/microtar.h"},
    deps=[
        git_repository(
            name="microtar_repo",
            url="https://github.com/rxi/microtar",
            branch="v0.1.0",
            path="dep/r/microtar",
        ),
    ],
)

clibrary(
    name="fatfs_lib",
    srcs=[
        "dep/r/fatfs/source/ff.c",
        "dep/r/fatfs/source/ffsystem.c",
        "dep/r/fatfs/source/ffunicode.c",
        "dep/r/fatfs/source/ff.h",
    ],
    hdrs={
        "ff.h": "dep/r/fatfs/source/ff.h",
        "ffconf.h": "dep/ffconf.h",
        "diskio.h": "dep/r/fatfs/source/diskio.h",
    },
    cflags=[
        "-Wno-pointer-sign",
        # Forces our own customised ffconf.h to be read in instead of the one
        # in the fatfs source code.
        "-include",
        ".obj/unix/dep/+fatfs_lib_hdr/ffconf.h",
    ],
    deps=[
        git_repository(
            name="fatfs_repo",
            url="https://github.com/davidgiven/fatfs",
            branch="R0.14b-fluxengine",
            path="dep/r/fatfs",
        )
    ],
)

git_repository(
    name="imhex_repo",
    url="https://github.com/davidgiven/ImHex",
    branch="master",
    path="dep/r/imhex",
),

git_repository(
    name="libromfs_repo",
    url="https://github.com/WerWolv/libromfs",
    branch="master",
    path="dep/r/libromfs",
),
