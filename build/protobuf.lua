definerule("proto_cc_library",
	{
		srcs = { type="targets", default={} },
		deps = { type="targets", default={} },
		commands = {
			type="strings",
			default={
				"rm -f %{outs[1]}",
				"$(AR) cqs %{outs[1]} %{ins}",
			},
		}
	},
	function (e)
		local ins = {}
		local outleaves = {}
		for _, src in fpairs(e.srcs) do
			local n = src:gsub("%.%w*$", "")
			ins[#ins+1] = src
			outleaves[#outleaves+1] = n..".pb.cc"
			outleaves[#outleaves+1] = n..".pb.h"
		end

		local protosrcs = normalrule {
			name = e.name.."_src",
			cwd = e.cwd,
			ins = ins,
			outleaves = outleaves,
			commands = {
				"%{PROTOC} --cpp_out=%{dir} %{ins}"
			}
		}

		local hdrs = {}
		for _, src in fpairs(e.srcs) do
			local n = src:gsub("%.%w*$", "")
			hdrs[n..".pb.h"] = protosrcs.dir.."/"..n..".pb.h"
		end
			
		local lib = clibrary {
			name = e.name,
			srcs = { protosrcs },
			hdrs = hdrs,
			dep_cflags = { "-I"..protosrcs.dir },
			dep_ldflags = { "-pthread" },
			vars = {
				["+cflags"] = { "-I"..protosrcs.dir, "-pthread" }
			}
		}

		return lib
	end
)


