local fnmatch_dep = {}
if vars.PLATFORM == "WINDOWS" then
	fnmatch_dep = "dep/emu+emu"
end

cprogram {
	name = "brother120tool",
	srcs = { "./brother120tool.cc" },
	deps = {
		"~+libfluxengine",
		fnmatch_dep,
	},
}

cprogram {
	name = "brother240tool",
	srcs = { "./brother240tool.cc" },
	deps = {
		"~+libfluxengine",
		fnmatch_dep,
	},
}

dependency {
	name = "sqlite3_dep",
	pkg_config = "sqlite3"
}

cprogram {
	name = "upgrade-flux-file",
	srcs = { "./upgrade-flux-file.cc" },
	deps = {
		"~+libfluxengine",
		"+sqlite3_dep"
	},
}

