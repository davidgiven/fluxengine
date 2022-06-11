cprogram {
	name = "fluxengine",
	srcs = { "src/*.cc" },
	deps = {
		".+libfluxengine",
		".+config_lib",
		".+protocol_lib",
		".+fmt_dep",
		".+protobuf_dep",
		".+zlib_dep",
		"src/formats+formats",
		"dep/agg+agg",
		"dep/stb+stb",
		"dep/libusbp+libusbp",
		".+libudev_dep",
	},
	vars = {
		["+cflags"] = { "-Ilib", "-Iarch", "-I." }
	}
}



