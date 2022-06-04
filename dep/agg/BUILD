package(default_visibility = ["//visibility:public"])

cc_library(
	name = "agg",
	hdrs = glob(["include/*.h"]),
	srcs = glob(["src/*.cpp", "src/*.h"]),
	copts = [
		"-Wno-reorder",
		"-Wno-deprecated-enum-enum-conversion",
		"-Wno-deprecated-enum-float-conversion",
	],
	includes = [
		"include",
	],
)

