package(default_visibility = ["//visibility:public"])

load("//:defs.bzl", "proto_encode", "objectify")

TEXTPBS = [
	"40track_drive",
    "acornadfs",
    "acorndfs",
    "aeslanier",
    "agat840",
    "amiga",
    "ampro",
    "apple2",
    "apple2_drive",
    "appledos",
    "atarist360",
    "atarist370",
    "atarist400",
    "atarist410",
    "atarist720",
    "atarist740",
    "atarist800",
    "atarist820",
    "brother120",
    "brother240",
    "commodore1541",
    "commodore1581",
    "eco1",
    "f85",
    "fb100",
    "hp9121",
    "hplif770",
    "ibm",
    "ibm1200",
    "ibm1232",
    "ibm1440",
    "ibm180",
    "ibm360",
    "ibm720",
    "bk800",
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
]

[
	[
		proto_encode(
			name = name + "_binpb",
			message = "ConfigProto",
			src = name + ".textpb",
			proto = "lib/config.proto",
			deps = [
				"//:fluxengine_proto",
				"@com_google_protobuf//:descriptor_proto"
			]
		),
		objectify(
			name = name,
			identifier = "formats_{}_pb".format(name),
			src = name + "_binpb"
		)
	]

	for name in TEXTPBS
]

genrule(
	name = "table",
	tools = [ "//scripts:mktable.sh" ],
	outs = [ "table.cc" ],
	cmd = "$(location //scripts:mktable.sh) formats {} > $@".format(" ".join(TEXTPBS))
)

cc_library(
	name = "formats",
	srcs = [ ":"+name for name in TEXTPBS ] + [ ":table" ]
)

