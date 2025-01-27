from build.ab import simplerule, Rule, Target
from build.c import cxxprogram, cxxlibrary
from build.pkg import package
import config

package(name="Qt5Widgets", package="Qt5Widgets")
package(name="Qt5Concurrent", package="Qt5Concurrent")


@Rule
def uic(self, name, src: Target = None, dest=None):
    simplerule(
        replaces=self,
        ins=[src],
        outs=["=" + dest],
        commands=["$(UIC) -g cpp -o {outs[0]} {ins[0]}"],
        label="UIC",
    )


for n in ["imager", "filer", "frame"]:
    uic(name=n + "_h", src="./" + n + ".ui", dest=n + ".h")

simplerule(
    name="resources_cc",
    ins=["./resources.qrc"],
    outs=["=resources.cc"],
    commands=["$(RCC) -g cpp --name resources -o {outs[0]} {ins[0]}"],
    label="RCC",
)

cxxlibrary(
    name="framelib",
    srcs=[
        "./main.cc",
        "./mainwindow.cc",
        "./drivecomponent.cc",
        "./formatcomponent.cc",
        "./fluxvisualiserwidget.cc",
        "./imagevisualiserwidget.cc",
        "./scene.cc",
        ".+resources_cc",
    ],
    hdrs={
        "globals.h": "./globals.h",
        "mainwindow.h": "./mainwindow.h",
        "frame.h": ".+frame_h",
        "fluxvisualiserwidget.h": "./fluxvisualiserwidget.h",
        "imagevisualiserwidget.h": "./imagevisualiserwidget.h",
    },
    cflags=["-fPIC"],
    deps=[
        "lib/config",
        "lib/core",
        "lib/data",
        "lib/external",
        "lib/algorithms",
        "lib/vfs",
        ".+Qt5Concurrent",
        ".+Qt5Widgets",
        "dep/verdigris",
    ],
)

cxxprogram(
    name="imager",
    srcs=[
        "./mainwindow-imager.cc",
        "./scene.cc",
        ".+imager_h",
    ],
    cflags=["-fPIC"],
    ldflags=["$(QT5_EXTRA_LIBS)"],
    deps=[
        ".+imager_h",
        "+fmt_lib",
        "+protobuf_lib",
        "+protocol",
        "+z_lib",
        ".+framelib",
        ".+Qt5Concurrent",
        ".+Qt5Widgets",
        "dep/adflib",
        "dep/fatfs",
        "dep/hfsutils",
        "dep/libusbp",
        "dep/verdigris",
        "extras+icons",
        "lib/config",
        "lib/core",
        "lib/data",
        "lib/external",
        "src/formats",
        "src/gui/drivetypes",
    ],
)

cxxprogram(
    name="filer",
    srcs=[
        "./mainwindow-filer.cc",
        ".+filer_h",
    ],
    cflags=["-fPIC"],
    ldflags=["$(QT5_EXTRA_LIBS)"],
    deps=[
        ".+filer_h",
        "+fmt_lib",
        "+protobuf_lib",
        "+protocol",
        "+z_lib",
        ".+framelib",
        ".+Qt5Concurrent",
        ".+Qt5Widgets",
        "dep/adflib",
        "dep/fatfs",
        "dep/hfsutils",
        "dep/libusbp",
        "dep/verdigris",
        "extras+icons",
        "lib/config",
        "lib/core",
        "lib/data",
        "lib/external",
        "src/formats",
        "src/gui/drivetypes",
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
            ".+imager",
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
            "cp $$(brew --prefix wxwidgets)/README.md $@/Contents/libs/wxWidgets.md",
            "cp $$(brew --prefix protobuf)/LICENSE $@/Contents/libs/protobuf.txt",
            "cp $$(brew --prefix fmt)/LICENSE.rst $@/Contents/libs/fmt.rst",
            "cp $$(brew --prefix libpng)/LICENSE $@/Contents/libs/libpng.txt",
            "cp $$(brew --prefix libjpeg)/README $@/Contents/libs/libjpeg.txt",
            "cp $$(brew --prefix abseil)/LICENSE $@/Contents/libs/abseil.txt",
            "cp $$(brew --prefix libtiff)/LICENSE.md $@/Contents/libs/libtiff.txt",
            "cp $$(brew --prefix zstd)/LICENSE $@/Contents/libs/zstd.txt",
        ],
        label="MKAPP",
    )
