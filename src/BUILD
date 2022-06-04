package(default_visibility = ["//visibility:public"])

load("//:defs.bzl", "cc_binary_both")

cc_binary_both(
	name = "fluxengine",
	srcs = glob(["*.cc", "*.h"]) + [
		"//:protocol.h"
	],
	deps = [
		"//:fluxengine_hdrs",
		"//:fluxengine_arch",
		"//:fluxengine_lib",
		"//:fluxengine_cc_proto",
		"//src/formats",
		"//dep/fmt",
		"//dep/agg",
		"//dep/libusbp",
		"//dep/stb",
	]
)

