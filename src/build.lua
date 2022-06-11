cprogram {
	name = "fluxengine",
	srcs = {
		"./fe-analysedriveresponse.cc",
		"./fe-analyselayout.cc",
		"./fe-inspect.cc",
		"./fe-rawread.cc",
		"./fe-rawwrite.cc",
		"./fe-read.cc",
		"./fe-rpm.cc",
		"./fe-seek.cc",
		"./fe-testbandwidth.cc",
		"./fe-testvoltages.cc",
		"./fe-write.cc",
		"./fluxengine.cc",
	},
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
	},
	vars = {
		["+cflags"] = { "-Ilib", "-Iarch", "-I." }
	}
}



