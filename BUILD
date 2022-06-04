package(default_visibility = ["//visibility:public"])

load("//:defs.bzl", "cc_binary_both")
load("@rules_proto//proto:defs.bzl", "proto_library")

exports_files([ "protocol.h" ])

proto_library(
    name = "fluxengine_proto",
    srcs = glob(["arch/*/*.proto", "lib/*.proto", "lib/*/*.proto"]),
    deps = [
        "@com_google_protobuf//:descriptor_proto"
    ]
)
  
cc_proto_library(
    name = "fluxengine_cc_proto",
    deps = [ ":fluxengine_proto" ]
)

cc_library(
    name = "fluxengine_arch",
    hdrs = glob(["arch/*/*.h"]),
    srcs = glob(["arch/*/*.cc"])
        + [ "protocol.h" ],
    deps = [
        ":fluxengine_cc_proto",
        ":fluxengine_hdrs",
        "//dep/fmt",
        "//dep/stb",
    ]
)

cc_library(
    name = "fluxengine_hdrs",
    hdrs = glob(["lib/*.h", "lib/*/*.h"]),
)

cc_library(
    name = "fluxengine_lib",
    srcs = glob(["lib/*.cc", "lib/*/*.cc", "lib/*/*.h"]) + [ "protocol.h" ],
    deps = [
        ":fluxengine_cc_proto",
        ":fluxengine_arch",
        ":fluxengine_hdrs",
        "//dep/fmt",
        "//dep/agg",
        "//dep/libusbp",
    ],
)

filegroup(
    name = "commands",
    srcs = [
        "//src:fluxengine_opt",
        "//src:fluxengine_dbg",
        "//tools:brother120tool_opt",
        "//tools:brother120tool_dbg",
        "//tools:brother240tool_opt",
        "//tools:brother240tool_dbg",
        "//tools:upgrade-flux-file_opt",
        "//tools:upgrade-flux-file_dbg",
    ]
)

# vim: sw=4 ts=4 et

