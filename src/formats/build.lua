FORMATS = {
	"40track_drive",
	"acornadfs",
	"acorndfs",
	"aeslanier",
	"agat840",
	"amiga",
	"ampro",
	"apple2_drive",
	"apple2",
	"appledos",
	"atarist360",
	"atarist370",
	"atarist400",
	"atarist410",
	"atarist720",
	"atarist740",
	"atarist800",
	"atarist820",
	"bk800",
	"brother120",
	"brother240",
	"commodore1541",
	"commodore1581",
	"eco1",
	"f85",
	"fb100",
	"hp9121",
	"hplif770",
	"ibm1200",
	"ibm1232",
	"ibm1440",
	"ibm180",
	"ibm360",
	"ibm720",
	"ibm",
	"mac400",
	"mac800",
	"micropolis143",
	"micropolis287",
	"micropolis315",
	"micropolis630",
	"mx",
	"n88basic",
	"northstar175",
	"northstar350",
	"northstar87",
	"prodos",
	"rx50",
	"shugart_drive",
	"tids990",
	"vgi",
	"victor9k_ds",
	"victor9k_ss",
	"zilogmcz",
}

cprogram {
	name = "protoencode",
	srcs = { "scripts/protoencode.cc" },
	deps = {
		"~+config_lib",
		"~+protobuf_dep",
		"~+fmt_dep",
	},
}

for _, format in pairs(FORMATS) do
	normalrule {
		name = format.."_cc_proto",
		ins = { "./"..format..".textpb", "+protoencode" },
		outleaves = { format..".cc" },
		commands = {
			"%{ins[2]} %{ins[1]} %{outs} formats_"..format.."_pb"
		}
	}
end

normalrule {
	name = "format_table",
	ins = { "scripts/mktable.sh" },
	outleaves = { "formats.cc" },
	commands = {
		"%{ins[1]} formats "..table.concat(FORMATS, " ").." > %{outs[1]}"
	}
}

clibrary {
	name = "formats",
	srcs = {
		"+format_table",
		map(FORMATS, function(f) return "+"..f.."_cc_proto" end),
	}
}

