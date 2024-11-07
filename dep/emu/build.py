from build.c import clibrary

clibrary(name="emu", srcs=["./fnmatch.c"], hdrs={"fnmatch.h": "./fnmatch.h"})
