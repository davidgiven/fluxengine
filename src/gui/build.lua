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
			"src/gui/main.cc",
			"src/gui/mainwindow.cc",
			"src/gui/visualisation.cc",
			"src/gui/layout.cpp",
		},
		deps = {
			"~+libfluxengine",
			"+wxwidgets_dep",
			"src/formats+formats",
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
