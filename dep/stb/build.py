from build.c import clibrary

clibrary(
    name="stb",
    srcs=["./stb_image_write.c"],
    hdrs={"stb_image_write.h": "./stb_image_write.h"},
)
