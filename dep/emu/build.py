from build.c import clibrary

clibrary(
    name="emu",
    srcs=["./fnmatch.c", "./charclass.h"],
    hdrs={"fnmatch.h": "./fnmatch.h"},
)
