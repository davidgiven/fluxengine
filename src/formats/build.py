from build.ab import normalrule, export
from build.c import cxxlibrary
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
    "q1",
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

cxxlibrary(
    name="formats",
    srcs=[".+table_cc"] + encoded,
    deps=["+lib", "lib+config_proto_lib"],
)

export(
    name="docs",
    items={
        f"doc/disk-{f}.md": normalrule(
            name=f"{f}_doc",
            ins=["scripts+mkdoc"],
            outs=[f"disk-{f}.md"],
            commands=["{ins[0]} " + f + " | tr -d '\\r' > {outs[0]}"],
            label="MKDOC",
        )
        for f in formats
    },
)
