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
		"~+libfluxengine",
		"src/formats+formats",
		"dep/agg+agg",
		"~+stb_dep",
	},
	vars = {
		["+cflags"] = { "-Ilib", "-Iarch", "-I." }
	}
}



