local wxwidgets_dep = wxwidgets {
	name = "wxwidgets_dep",
	static = false,
	modules = { "core", "base" },
	optional = true
}

if wxwidgets_dep.found then
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
else
	empty {
		name = "fluxengine"
	}
end
