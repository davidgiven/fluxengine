from build.ab import emit
from build.c import clibrary
from build.pkg import package
from config import windows, osx, unix

srcs = [
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
]
deps = []
ldflags = []

if windows:
    srcs += [
        "./src/windows/async_in_transfer_windows.c",
        "./src/windows/device_instance_id_windows.c",
        "./src/windows/device_windows.c",
        "./src/windows/error_windows.c",
        "./src/windows/generic_handle_windows.c",
        "./src/windows/generic_interface_windows.c",
        "./src/windows/interface_windows.c",
        "./src/windows/list_windows.c",
        "./src/windows/serial_port_windows.c",
    ]
    ldflags += ["-lsetupapi", "-lwinusb", "-lole32", "-luuid"]
elif osx:
    srcs += [
        "./src/mac/async_in_transfer_mac.c",
        "./src/mac/device_mac.c",
        "./src/mac/error_mac.c",
        "./src/mac/generic_handle_mac.c",
        "./src/mac/generic_interface_mac.c",
        "./src/mac/iokit_mac.c",
        "./src/mac/list_mac.c",
        "./src/mac/serial_port_mac.c",
    ]
else:
    package(name="udev_lib", package="libudev")
    srcs += [
        "./src/linux/async_in_transfer_linux.c",
        "./src/linux/device_linux.c",
        "./src/linux/error_linux.c",
        "./src/linux/generic_handle_linux.c",
        "./src/linux/generic_interface_linux.c",
        "./src/linux/list_linux.c",
        "./src/linux/serial_port_linux.c",
        "./src/linux/udev_linux.c",
        "./src/linux/usbfd_linux.c",
    ]
    deps += [".+udev_lib"]

clibrary(
    name="libusbp",
    srcs=srcs,
    cflags=[
        "-Idep/libusbp/include",
        "-Idep/libusbp/src",
        "-Wno-deprecated-declarations",
    ],
    caller_ldflags=ldflags,
    deps=deps,
    hdrs={
        "libusbp_internal.h": "./src/libusbp_internal.h",
        "libusbp_config.h": "./include/libusbp_config.h",
        "libusbp.hpp": "./include/libusbp.hpp",
        "libusbp.h": "./include/libusbp.h",
    },
)
