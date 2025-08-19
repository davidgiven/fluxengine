from build.c import cxxlibrary
from glob import glob

cxxlibrary(
    name="lexy",
    srcs=[],
    hdrs={
        h: f"./include/{h}"
        for h in glob("**/*.hpp", root_dir="dep/lexy/include", recursive=True)
    },
)
