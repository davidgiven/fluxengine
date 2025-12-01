from build.c import cxxlibrary
from build.utils import glob

cxxlibrary(
    name="lexy",
    srcs=[],
    hdrs={
        h: f"./include/{h}"
        for h in glob(
            ["**/*.hpp"], dir="dep/lexy/include", relative_to="dep/lexy/include"
        )
    },
)
