from build.protobuf import proto, protocc
from build.c import cxxlibrary

proto(name="proto", srcs=["./usb.proto"], deps=["lib/config+common_proto"])
protocc(
    name="proto_lib", srcs=[".+proto"], deps=["lib/config+common_proto_lib"]
)

cxxlibrary(
    name="usb",
    srcs=[
        "./applesauceusb.cc",
        "./fluxengineusb.cc",
        "./greaseweazleusb.cc",
        "./serial.cc",
        "./usb.cc",
        "./usbfinder.cc",
    ],
    hdrs={"lib/usb/usb.h": "./usb.h", "lib/usb/usbfinder.h": "./usbfinder.h"},
    deps=["lib/core", "lib/config", "lib/external", "dep/libusbp", "+protocol"],
)
