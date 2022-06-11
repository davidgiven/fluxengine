cprogram {
	name = "brother120tool",
	srcs = { "./brother120tool.cc" },
	deps = { "~+libfluxengine" },
}

cprogram {
	name = "brother240tool",
	srcs = { "./brother240tool.cc" },
	deps = { "~+libfluxengine" },
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

