package(default_visibility = ["//visibility:public"])

load(":toolchain.bzl", "cc_toolchain_config")

exports_files([ "mktable.sh", "proto_encode.cc" ])

filegroup(name = "empty")

cc_toolchain_config(name = "mingw32_toolchain_config")

cc_toolchain_suite(
    name = "mingw-toolchain",
    toolchains = {
        'i386_windows': ":mingw-toolchain-x32"
    },
)

cc_toolchain(
    name = "mingw-toolchain-x32",
    toolchain_identifier = "mingw32-toolchain",
    toolchain_config = ":mingw32_toolchain_config",
    all_files = ":empty",
    compiler_files = ":empty",
    dwp_files = ":empty",
    linker_files = ":empty",
    objcopy_files = ":empty",
    strip_files = ":empty",
    supports_param_files = 0,
)