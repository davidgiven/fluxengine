from build.ab import normalrule
from build.c import clibrary
from scripts.build import protoencode

drivetypes = [
    "35_40",
    "35_80",
    "525_40M",
    "525_40",
    "525_80M",
    "525_80",
    "8_38",
    "8_77",
    "apple2",
]

normalrule(
    name="drivetypes_cc",
    ins=[f"./{name}.textpb" for name in drivetypes],
    deps=["scripts/mktable.sh"],
    outs=["table.cc"],
    commands=[
        "sh scripts/mktable.sh drivetypes "
        + (" ".join(drivetypes))
        + " > {outs}"
    ],
    label="MKTABLE",
)

encoded = [
    protoencode(
        name=f"{name}_cc",
        srcs=[f"./{name}.textpb"],
        proto="ConfigProto",
        symbol=f"drivetypes_{name}_pb",
    )
    for name in drivetypes
]

clibrary(name="drivetypes", srcs=[".+drivetypes_cc"] + encoded)