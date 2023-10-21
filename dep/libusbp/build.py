from build.c import clibrary

clibrary(
    name="libusbp",
    srcs=[
        "./src/async_in_pipe.c",
        "./src/error.c",
        "./src/error_hresult.c",
        "./src/find_device.c",
        "./src/list.c",
        "./src/pipe_id.c",
        "./src/string.c",
        "./src/libusbp_internal.h",
        "./include/libusbp_config.h",
        "./include/libusbp.h",
    ],
    cflags=["-Idep/libusbp/include", "-Idep/libusbp/src"],
    hdrs={
        "libusbp_internal.h": "./src/libusbp_internal.h",
        "libusbp_config.h": "./include/libusbp_config.h",
        "libusbp.hpp": "./include/libusbp.hpp",
        "libusbp.h": "./include/libusbp.h",
    },
)
