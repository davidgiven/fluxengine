from build.ab import emit
from build.c import cxxprogram

emit(
    """
WX_CONFIG ?= wx-config
ifneq ($(strip $(shell command -v $(WX_CONFIG) >/dev/null 2>&1; echo $$?)),0)
$(error Required binary 'wx-config' not found.)
endif

WX_CFLAGS := $(shell $(WX_CONFIG) --cxxflags core base adv aui richtext)
WX_LDFLAGS := $(shell $(WX_CONFIG) --libs core base adv aui richtext)
"""
)

cxxprogram(
    name="gui",
    srcs=[
        "./browserpanel.cc",
        "./customstatusbar.cc",
        "./explorerpanel.cc",
        "./filesystemmodel.cc",
        "./fileviewerwindow.cc",
        "./fluxviewercontrol.cc",
        "./fluxviewerwindow.cc",
        "./histogramviewer.cc",
        "./iconbutton.cc",
        "./idlepanel.cc",
        "./imagerpanel.cc",
        "./jobqueue.cc",
        "./main.cc",
        "./mainwindow.cc",
        "./texteditorwindow.cc",
        "./textviewerwindow.cc",
        "./visualisationcontrol.cc",
        "./layout.cpp",
    ],
    cflags=["$(WX_CFLAGS)"],
    ldflags=["$(WX_LDFLAGS)"],
    deps=[
        "+fl2_proto_lib",
        "+protocol",
        "dep/adflib",
        "dep/fatfs",
        "dep/hfsutils",
        "dep/libusbp",
        "extras+icons",
        "+lib",
        "lib+config_proto_lib",
        "src/formats",
        "src/gui/drivetypes",
        "+z_lib",
        "+fmt_lib",
        "+protobuf_lib",
    ],
)
