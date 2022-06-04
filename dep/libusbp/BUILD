package(default_visibility = ["//visibility:public"])

cc_library(
	name = "libusbp_headers",
	hdrs = glob(["src/*.h"]),
	strip_include_prefix = "./src"
)

cc_library(
	name = "libusbp",
	hdrs = glob(["include/*.h", "include/*.hpp"]),
	srcs = glob(["src/*.c"]) +
		select({
			"@bazel_tools//src/conditions:windows": glob(["src/windows/*.c"]),
			"@bazel_tools//src/conditions:darwin": glob(["src/mac/*.c"]),
			 "//conditions:default": glob(["src/linux/*.c"]),
    }),
	linkopts = 
		select({
			"@bazel_tools//src/conditions:windows": [],
			"@bazel_tools//src/conditions:darwin": [],
			 "//conditions:default":
				 [ "-ludev" ]
		}),
	deps = [ ":libusbp_headers" ],
	strip_include_prefix = "./include"
)

