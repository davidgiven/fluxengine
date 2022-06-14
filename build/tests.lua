definerule("test",
	{
		srcs = { type="targets", default={} },
	},
	function (e)
		if vars.TESTS == "yes" then
			normalrule {
				name = e.name,
				ins = e.srcs,
				outleaves = { "log.txt" },
				commands = {
					"%{ins} > %{outs}",
				}
			}
		end
	end
)

