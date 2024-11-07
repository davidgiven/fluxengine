from build.ab import simplerule
from build.c import cxxlibrary
from scripts.build import protoencode

drivetypes = [
    "40track",
    "80track",
    "apple2",
]

simplerule(
    name="drivetypes_table_cc",
    ins=[f"./{name}.textpb" for name in drivetypes],
    deps=["scripts/mktable.sh"],
    outs=["=table.cc"],
    commands=[
        "sh scripts/mktable.sh drivetypes "
        + (" ".join(drivetypes))
        + " > {outs}"
    ],
    label="MKTABLE",
)


protoencode(
    name="drivetypes_cc",
    srcs={name: f"./{name}.textpb" for name in drivetypes},
    proto="ConfigProto",
    symbol="drivetypes",
)

cxxlibrary(
    name="drivetypes",
    srcs=[".+drivetypes_cc", ".+drivetypes_table_cc"],
    deps=["lib/core", "lib/config"],
)
