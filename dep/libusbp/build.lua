clibrary {
	name = "libusbp",
	srcs = {
		"./src/*.c",
		"./src/linux/*.c",
	},
	hdrs = {
		["libusbp_config.h"] = "./include/libusbp_config.h",
		["libusbp.h"] = "./include/libusbp.h",
		["libusbp.hpp"] = "./include/libusbp.hpp",
	},
	vars = {
		["+cflags"] = {
			"-I"..cwd().."/include",
			"-I"..cwd().."/src",
		}
	}
}

