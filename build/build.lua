local OBJDIR = "$(OBJDIR)"

local function objdir(e)
	return concatpath(OBJDIR, e.cwd, e.name)
end

definerule("normalrule",
	{
		ins = { type="targets" },
		deps = { type="targets", default={} },
		outleaves = { type="strings" },
		label = { type="string", optional=true },
		objdir = { type="string", optional=true },
		commands = { type="strings" },
	},
	function (e)
		local dir = e.objdir or objdir(e)
		local realouts = {}
		for _, v in pairs(e.outleaves) do
			realouts[#realouts+1] = concatpath(dir, v)
		end

		local vars = inherit(e.vars, {
			dir = dir
		})

		local result = simplerule {
			name = e.name,
			ins = e.ins,
			deps = e.deps,
			outs = realouts,
			label = e.label,
			commands = e.commands,
			vars = vars,
		}
		result.dir = dir
		return result
	end
)

local function is_clike(f)
	return f:find("%.c$") or f:find("%.cc$") or f:find("%.cpp$")
end

definerule("cfile",
	{
		srcs = { type="targets" },
		deps = { type="targets", default={} }
	},
	function (e)
		local cflags = e.vars.cflags
		local cxxflags = e.vars.cxxflags
		for _, target in ipairs(targetsof(e.deps)) do
			if target.is.clibrary then
				cflags = concat(cflags, target.dep_cflags)
				cxxflags = concat(cxxflags, target.dep_cxxflags)
			end
		end

		local src = filter(filenamesof(e.srcs), is_clike)
		local cmd
		local cxx = false
		if src[1]:find("%.c$") then
			cmd = "$(CC) -c -o %{outs[1]} %{ins[1]} %{hdrpaths} %{cflags}"
		else
			cmd = "$(CXX) -c -o %{outs[1]} %{ins[1]} %{hdrpaths} %{cflags} %{cxxflags}"
			cxx = true
		end

		local outleaf = basename(e.name)..".o"
		local rule = normalrule {
			name = e.name,
			cwd = e.cwd,
			ins = e.srcs,
			deps = e.deps,
			outleaves = {outleaf},
			label = e.label,
			commands = cmd,
			vars = {
				hdrpaths = {},
				cflags = cflags,
				cxxflags = cxxflags,
			}
		}

		rule.is.cxxfile = cxx
		return rule
	end
)

local function do_cfiles(e)
	local outs = {}
	local srcs = filenamesof(e.srcs)
	for _, f in ipairs(sorted(filter(srcs, is_clike))) do
		local ofile
		if f:find(OBJDIR, 1, true) == 1 then
			ofile = e.name.."/"..f:sub(#OBJDIR+1)..".o"
		else
			ofile = e.name.."/"..f..".o"
		end
		outs[#outs+1] = cfile {
			name = ofile,
			srcs = { f },
			deps = e.deps
		}
	end
	return outs
end

definerule("clibrary",
	{
		srcs = { type="targets", default={} },
		deps = { type="targets", default={} },
		hdrs = { type="targets", default={} },
		dep_cflags = { type="strings", default={} },
		dep_cxxflags = { type="strings", default={} },
		dep_ldflags = { type="strings", default={} },
		dep_libs = { type="strings", default={} },
	},
	function (e)
		local ins = do_cfiles(e)
		local cxx = false
		for _, f in ipairs(ins) do
			if f.is.cxxfile then
				cxx = true
				break
			end
		end

		local commands = {
			"rm -f %{outs[1]} && $(AR) cqs %{outs[1]} %{ins}"
		}

		local deps = {}
		for k, v in pairs(e.hdrs) do
			deps[#deps+1] = v
			if type(k) == "number" then
				v = filenamesof(v)
				for _, v in ipairs(v) do
					if not startswith(e.cwd, v) then
						error(string.format("filename '%s' is not local to '%s' --- "..
							"you'll have to specify the output filename manually", v, e.cwd))
					end
					commands[#commands+1] = string.format("install -D %s %%{dir}/%s", v, v:gsub("^"..e.cwd, ""))
				end
			else
				v = filenamesof(v)
				if #v ~= 1 then
					error("each mapped hdrs item can only cope with a single file")
				end
				commands[#commands+1] = string.format("install -D %s %%{dir}/%s", v[1], k)
			end
		end

		local lib = normalrule {
			name = e.name,
			cwd = e.cwd,
			ins = sorted(filenamesof(ins)),
			deps = deps,
			outleaves = { e.name..".a" },
			label = e.label,
			commands = sorted(commands),
		}
		
		lib.dep_cflags = concat(e.dep_cflags, "-I"..lib.dir)
		lib.dep_cxxflags = e.dep_cxxflags
		lib.dep_ldflags = e.dep_ldflags
		lib.dep_libs = concat(e.dep_libs, filenamesof(lib))
		lib.dep_cxx = cxx

		for _, d in pairs(targetsof(e.deps)) do
			lib.dep_cflags = concat(lib.dep_cflags, d.dep_cflags)
			lib.dep_cxxflags = concat(lib.dep_cxxflags, d.dep_cxxflags)
			lib.dep_ldflags = concat(lib.dep_ldflags, d.dep_ldflags)
			lib.dep_libs = concat(lib.dep_libs, d.dep_libs)
			lib.dep_cxx = lib.dep_cxx or d.dep_cxx
		end

		return lib
	end
)

definerule("cprogram",
	{
		srcs = { type="targets", default={} },
		deps = { type="targets", default={} },
	},
	function (e)
		local deps = e.deps
		local ins = {}
		local cxx = false

		if (#e.srcs > 0) then
			ins = do_cfiles(e)
			for _, obj in pairs(ins) do
				if obj.is.cxxfile then
					cxx = true
				end
			end
		end

		local libs = {}
		local cflags = {}
		local cxxflags = {}
		local ldflags = {}
		for _, lib in pairs(e.deps) do
			cflags = concat(cflags, lib.dep_cflags)
			cxxflags = concat(cxxflags, lib.dep_cxxflags)
			ldflags = concat(ldflags, lib.dep_ldflags)
			libs = concat(libs, lib.dep_libs)
			cxx = cxx or lib.dep_cxx
		end

		local command
		if cxx then
			command = "$(CXX) $(LDFLAGS) %{ldflags} -o %{outs[1]} %{ins} %{libs}"
		else
			command = "$(CC) $(LDFLAGS) %{ldflags} -o %{outs[1]} %{ins} %{libs}"
		end

		return normalrule {
			name = e.name,
			cwd = e.cwd,
			deps = deps,
			ins = ins,
			outleaves = { e.name },
			commands = { command },
			vars = {
				cflags = cflags,
				cxxflags = cxxflags,
				ldflags = ldflags,
				libs = libs,
			}
		}
	end
)

