vars.cflags = { "$(CFLAGS)" }
vars.cxxflags = { "$(CXXFLAGS)" }
vars.ldflags = { "-pthread" }

include "build/protobuf.lua"
include "build/dependency.lua"

dependency {
	name = "fmt_dep",
	pkg_config = "fmt",
	fallback = "dep/fmt+fmt"
}

dependency {
	name = "protobuf_dep",
	pkg_config = "protobuf"
}

dependency {
	name = "zlib_dep",
	pkg_config = "zlib"
}

dependency {
	name = "libudev_dep",
	pkg_config = "libudev"
}

proto_cc_library {
	name = "config_lib",
	srcs = {
		"lib/*.proto",
		"lib/*/*.proto",
		"arch/*/*.proto",
	}
}

clibrary {
	name = "protocol_lib",
	hdrs = { "protocol.h" }
}

clibrary {
	name = "libfluxengine",
	srcs = {
		"arch/*/*.cc",
		"lib/*/*.cc",
		"lib/*.cc",
		"protocol.h",
	},
	deps = {
		"+config_lib",
		"+protocol_lib",
		"+fmt_dep",
		"+protobuf_dep",
		"+zlib_dep",
		"dep/libusbp+libusbp",
	},
	vars = {
		["+cflags"] = { "-Ilib", "-Iarch", "-I." }
	}
}

installable {
	name = "all",
	map = {
		["fluxengine"] = "src+fluxengine",
		["fluxengine-gui"] = "src/gui+fluxengine",
	}
}

