wxwidgets {
	name = "wxwidgets_dep",
	static = false,
	modules = { "core", "base" }
}

cprogram {
	name = "fluxengine",
	srcs = {
		"./*.cpp",
		"./*.cc"
	},
	deps = {
		".+libfluxengine",
		"src/formats+formats",
		".+config_lib",
		"+wxwidgets_dep",
		".+zlib_dep",
		".+libudev_dep",
		".+protobuf_dep",
		".+fmt_dep",
		"dep/libusbp+libusbp",
	},
	vars = {
		["+cflags"] = { "-Ilib", "-Iarch", "-I." }
	}
}

