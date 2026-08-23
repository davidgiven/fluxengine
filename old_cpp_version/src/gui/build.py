from build.ab import simplerule, G
from build.c import cxxprogram
from build.utils import shell, does_command_exist
from glob import glob
import config

G.setdefault("WX_CONFIG", "wx-config")
assert does_command_exist(G.WX_CONFIG), "Required binary 'wx-config' not found"

G.setdefault(
    "WX_CFLAGS",
    shell(f"{G.WX_CONFIG} --cxxflags base adv aui richtext core"),
)
G.setdefault(
    "WX_LDFLAGS",
    shell(f"{G.WX_CONFIG} --libs base adv aui richtext core"),
)

extrasrcs = ["./layout.cpp"]
if config.windows:
    extrasrcs += [
        simplerule(
            name="rc",
            ins=["./windres.rc"],
            outs=["=rc.o"],
            deps=["./manifest.xml", "extras+fluxengine_ico"],
            commands=["$(WINDRES) $[ins[0]] $[outs[0]]"],
            label="WINDRES",
        )
    ]

cxxprogram(
    name="gui",
    srcs=glob("src/gui/*.cc") + glob("src/gui/*.h") + extrasrcs,
    cflags=["$(WX_CFLAGS)"],
    ldflags=["$(WX_LDFLAGS)"],
    deps=[
        "lib/external+fl2_proto_lib",
        "+protocol",
        "dep/adflib",
        "dep/fatfs",
        "dep/hfsutils",
        "dep+libusbp_lib",
        "extras+icons",
        "lib/core",
        "lib/data",
        "lib/vfs",
        "lib/config",
        "arch",
        "src/formats",
        "src/gui/drivetypes",
        "+z_lib",
        "dep+fmt_lib",
        "+protobuf_lib",
    ],
)

if config.osx:
    simplerule(
        name="fluxengine_app_zip",
        ins=[
            ".+gui",
            "extras+fluxengine_icns",
            "extras+fluxengine_template",
        ],
        outs=["=FluxEngine.app.zip"],
        commands=[
            "rm -rf $[outs[0]]",
            "unzip -q $[ins[2]]",  # creates FluxEngine.app
            "cp $[ins[0]] FluxEngine.app/Contents/MacOS/fluxengine-gui",
            "mkdir -p FluxEngine.app/Contents/Resources",
            "cp $[ins[1]] FluxEngine.app/Contents/Resources/FluxEngine.icns",
            "dylibbundler -of -x FluxEngine.app/Contents/MacOS/fluxengine-gui -b -d FluxEngine.app/Contents/libs -cd > /dev/null",
            "cp $$(brew --prefix wxwidgets)/README.md FluxEngine.app/Contents/libs/wxWidgets.md",
            "cp $$(brew --prefix protobuf)/LICENSE FluxEngine.app/Contents/libs/protobuf.txt",
            "cp $$(brew --prefix fmt)/LICENSE* FluxEngine.app/Contents/libs/fmt.rst",
            "cp $$(brew --prefix libpng)/LICENSE FluxEngine.app/Contents/libs/libpng.txt",
            "cp $$(brew --prefix libjpeg)/README FluxEngine.app/Contents/libs/libjpeg.txt",
            "cp $$(brew --prefix abseil)/LICENSE FluxEngine.app/Contents/libs/abseil.txt",
            "cp $$(brew --prefix libtiff)/LICENSE.md FluxEngine.app/Contents/libs/libtiff.txt",
            "cp $$(brew --prefix zstd)/LICENSE FluxEngine.app/Contents/libs/zstd.txt",
            "zip -rq $[outs[0]] FluxEngine.app",
        ],
        label="MKAPP",
    )

    simplerule(
        name="fluxengine_pkg",
        ins=[
            ".+fluxengine_app_zip",
        ],
        outs=["=FluxEngine.pkg"],
        commands=[
            "unzip -q $[ins[0]]",
            "pkgbuild --quiet --install-location /Applications --component FluxEngine.app $[outs[0]]",
        ],
        label="MKPKG",
    )
