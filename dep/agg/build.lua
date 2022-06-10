clibrary {
	name = "agg",
	srcs = { "./src/*.cpp" },
	dep_cflags = { "-Idep/agg/include" },
	vars = {
		["+cflags"] = { "-Idep/agg/include" }
	}
}

