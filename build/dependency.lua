local function unnl(s)
	s = (s:gsub("[\n\r]+", " "))
	if s == "" then
		return nil
	end
	return s
end

local NOSTDERR = "2>/dev/null"

definerule("dependency",
	{
		pkg_config = { type="string", optional=true },
		fallback = { type="targets", optional=true }
	},
	function (e)
		local fallback = e.fallback and targetsof(e.fallback) or {}
		if #fallback > 1 then
			error("fallback must be a single target")
		end

		local cflags, _, _, e1 = shell(vars.PKG_CONFIG, "--silence-errors", "--cflags", e.pkg_config)
		local libs, _, _, e2 = shell(vars.PKG_CONFIG, "--silence-errors", "--libs", e.pkg_config)
		local version, _, _, e3 = shell(vars.PKG_CONFIG, "--silence-errors", "--modversion", e.pkg_config)
		if (e1 ~= 0) or (e2 ~= 0) or (e3 ~= 0) then
			if #fallback == 0 then
				error(string.format("required dependency '%s' missing", e.pkg_config))
			end
			print("dependency ", e.pkg_config, ": internal")
			return inherit(fallback[1], {})
		else
			print("dependency ", e.pkg_config, ": pkg-config ", unnl(version))
			return {
				is = { clibrary = true },
				dep_cflags = { unnl(cflags) },
				dep_libs = { unnl(libs) },
			}
		end
	end
)


definerule("wxwidgets",
	{
		static = { type="boolean", default=false },
		modules = { type="strings", default={} },
		optional = { type="boolean", default=false },
	},
	function (e)
		local static = e.static and "--static=yes" or ""
		local modules = table.concat(e.modules, " ")
		local cxxflags, _, _, e1 = shell(vars.WX_CONFIG, static, "--cxxflags")
		local libs, _, _, e2 = shell(vars.WX_CONFIG, static, "--libs", modules)
		local version, _, _, e3 = shell(vars.WX_CONFIG, "--version", e.pkg_config)
		if (e1 ~= 0) or (e2 ~= 0) or (e3 ~= 0) then
			if e.optional then
				print("dependency wxwidgets: missing (but optional)")
				return {
					is = { clibrary = true },
					found = false
				}
			end
			error("required dependency 'wxwidgets' missing")
		else
			print("dependency wxwidgets: wx-config ", unnl(version))
			return {
				is = { clibrary = true },
				dep_cxxflags = unnl(cxxflags),
				dep_libs = unnl(libs),
				found = true
			}
		end
	end
)

