from build.ab import emit, simplerule
from build.c import cxxprogram
import config

emit(
    """
WX_CONFIG ?= wx-config
ifneq ($(strip $(shell command -v $(WX_CONFIG) >/dev/null 2>&1; echo $$?)),0)
WX_CFLAGS = $(error Required binary 'wx-config' not found.)
WX_LDFLAGS = $(error Required binary 'wx-config' not found.)
else
WX_CFLAGS := $(shell $(WX_CONFIG) --cxxflags core base adv aui richtext)
WX_LDFLAGS := $(shell $(WX_CONFIG) --libs core base adv aui richtext)
endif
"""
)

extrasrcs = []
if config.windows:
    extrasrcs += [
        simplerule(
            name="rc",
            ins=["./windres.rc"],
            outs=["=rc.o"],
            deps=["./manifest.xml", "extras+fluxengine_ico"],
            commands=["$(WINDRES) {ins[0]} {outs[0]}"],
            label="WINDRES",
        )
    ]

cxxprogram(
    name="gui",
    srcs=[
        "./browserpanel.cc",
        "./context.cc",
        "./context.h",
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
    ]
    + extrasrcs,
    cflags=["$(WX_CFLAGS)"],
    ldflags=["$(WX_LDFLAGS)"],
    deps=[
        "lib/external+fl2_proto_lib",
        "+protocol",
        "dep/adflib",
        "dep/fatfs",
        "dep/hfsutils",
        "dep/libusbp",
        "extras+icons",
        "lib/core",
        "lib/data",
        "lib/vfs",
        "lib/config",
        "arch",
        "src/formats",
        "src/gui/drivetypes",
        "+z_lib",
        "+fmt_lib",
        "+protobuf_lib",
    ],
)

if config.osx:
    simplerule(
        name="fluxengine_pkg",
        ins=[".+fluxengine_app"],
        outs=["=FluxEngine.pkg"],
        commands=[
            "pkgbuild --quiet --install-location /Applications --component {ins[0]} {outs[0]}"
        ],
        label="PKGBUILD",
    )

    simplerule(
        name="fluxengine_app",
        ins=[
            ".+gui",
            "extras+fluxengine_icns",
            "extras/FluxEngine.app.template/",
        ],
        outs=["=FluxEngine.app"],
        commands=[
            "rm -rf {outs[0]}",
            "cp -a {ins[2]} {outs[0]}",
            "touch {outs[0]}",
            "cp {ins[0]} {outs[0]}/Contents/MacOS/fluxengine-gui",
            "mkdir -p {outs[0]}/Contents/Resources",
            "cp {ins[1]} {outs[0]}/Contents/Resources/FluxEngine.icns",
            "dylibbundler -of -x {outs[0]}/Contents/MacOS/fluxengine-gui -b -d {outs[0]}/Contents/libs -cd > /dev/null",
            "cp $$(brew --prefix wxwidgets)/README.md {outs[0]}/Contents/libs/wxWidgets.md",
            "cp $$(brew --prefix protobuf)/LICENSE {outs[0]}/Contents/libs/protobuf.txt",
            "cp $$(brew --prefix fmt)/LICENSE* {outs[0]}/Contents/libs/fmt.rst",
            "cp $$(brew --prefix libpng)/LICENSE {outs[0]}/Contents/libs/libpng.txt",
            "cp $$(brew --prefix libjpeg)/README {outs[0]}/Contents/libs/libjpeg.txt",
            "cp $$(brew --prefix abseil)/LICENSE {outs[0]}/Contents/libs/abseil.txt",
            "cp $$(brew --prefix libtiff)/LICENSE.md {outs[0]}/Contents/libs/libtiff.txt",
            "cp $$(brew --prefix zstd)/LICENSE {outs[0]}/Contents/libs/zstd.txt",
        ],
        label="MKAPP",
    )
