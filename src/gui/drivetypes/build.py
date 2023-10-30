from build.ab import normalrule
from build.c import cxxlibrary
from scripts.build import protoencode

drivetypes = [
    "40track",
    "80track",
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

cxxlibrary(name="drivetypes", srcs=[".+drivetypes_cc"] + encoded)
