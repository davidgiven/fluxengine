from build.c import clibrary

clibrary(
    name="alphanum",
    srcs=[],
    hdrs={"dep/alphanum/alphanum.h": "./alphanum.h"},
)

