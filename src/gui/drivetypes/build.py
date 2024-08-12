from build.ab import normalrule
from build.c import cxxlibrary
from scripts.build import protoencode

drivetypes = [
    "40track",
    "80track",
    "apple2",
]

protoencode(
    name="drivetypes_cc",
    srcs={name: f"./{name}.textpb" for name in drivetypes},
    proto="ConfigProto",
    symbol="drivetypes",
)

cxxlibrary(name="drivetypes", srcs=[".+drivetypes_cc"], deps=["+lib"])
