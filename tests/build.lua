TESTS = {
	["agg"] = { "dep/agg+agg", "dep/stb+stb", "~+zlib_dep" },
	["amiga"] = {},
	["bitaccumulator"] = {},
	["bytes"] = {},
	["compression"] = {},
	["csvreader"] = {},
	["flags"] = {},
	["fluxmapreader"] = {},
	["fluxpattern"] = {},
	["fmmfm"] = {},
	["greaseweazle"] = {},
	["kryoflux"] = {},
	["ldbs"] = {},
	--["proto"] = {},
	["utils"] = {},
}

for name, deps in pairs(TESTS) do
	cprogram {
		name = name.."_bin",
		srcs = { "./"..name..".cc" },
		deps = {
			"~+libfluxengine",
			"dep/snowhouse+snowhouse",
			deps
		},
	}

	test {
		name = name.."_test",
		srcs = { "+"..name.."_bin" }
	}
end

