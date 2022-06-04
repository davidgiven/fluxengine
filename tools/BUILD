package(default_visibility = ["//visibility:public"])

load("//:defs.bzl", "cc_binary_both")

cc_binary_both(
    name = "brother120tool",
    srcs = [ "brother120tool.cc" ],
    deps = [
        "//:fluxengine_hdrs",
        "//:fluxengine_lib"
    ]
)

cc_binary_both(
    name = "brother240tool",
    srcs = [ "brother240tool.cc" ],
    deps = [
        "//:fluxengine_hdrs",
        "//:fluxengine_lib"
    ]
)

cc_binary_both(
    name = "upgrade-flux-file",
    srcs = [ "upgrade-flux-file.cc" ],
	linkopts = [
		"-lsqlite3"
	],
    deps = [
        "//:fluxengine_hdrs",
        "//:fluxengine_lib"
    ]
)


