from build.ab import normalrule
from build.c import clibrary
from scripts.build import protoencode

formats = [
    "40track_drive",
    "acornadfs",
    "acorndfs",
    "aeslanier",
    "agat",
    "amiga",
    "ampro",
    "apple2_drive",
    "apple2",
    "atarist",
    "bk",
    "brother",
    "commodore",
    "eco1",
    "epsonpf10",
    "f85",
    "fb100",
    "hplif",
    "ibm",
    "icl30",
    "mac",
    "micropolis",
    "ms2000",
    "mx",
    "n88basic",
    "northstar",
    "psos",
    "rolandd20",
    "rx50",
    "shugart_drive",
    "smaky6",
    "tids990",
    "tiki",
    "victor9k",
    "zilogmcz",
]

normalrule(
    name="table_cc",
    ins=[f"./{name}.textpb" for name in formats],
    deps=["scripts/mktable.sh"],
    outs=["table.cc"],
    commands=[
        "sh scripts/mktable.sh formats " + (" ".join(formats)) + " > {outs}"
    ],
    label="MKTABLE",
)

encoded = [
    protoencode(
        name=f"{name}_cc",
        srcs=[f"./{name}.textpb"],
        proto="ConfigProto",
        symbol=f"formats_{name}_pb",
    )
    for name in formats
]

clibrary(name="formats", srcs=[".+table_cc"] + encoded, deps=["+lib"])
