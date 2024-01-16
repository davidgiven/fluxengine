from build.ab import normalrule, Rule, Target
from build.c import cxxprogram, cxxlibrary
from build.pkg import package
import config

package(name="Qt5Widgets", package="Qt5Widgets")
package(name="Qt5Concurrent", package="Qt5Concurrent")


@Rule
def uic(self, name, src: Target = None, dest=None):
    normalrule(
        replaces=self,
        ins=[src],
        outs=[dest],
        commands=["uic -g cpp -o {outs[0]} {ins[0]}"],
        label="UIC",
    )


for n in ["userinterface", "driveConfigurationForm", "fluxConfigurationForm"]:
    uic(name=n + "_h", src="./" + n + ".ui", dest=n + ".h")

normalrule(
    name="resources_cc",
    ins=["./resources.qrc"],
    outs=["resources.cc"],
    commands=["rcc -g cpp --name resources -o {outs[0]} {ins[0]}"],
    label="RCC",
)

cxxlibrary(
    name="userinterface",
    srcs=[".+resources_cc"],
    hdrs={
        "userinterface.h": ".+userinterface_h",
        "driveConfigurationForm.h": ".+driveConfigurationForm_h",
        "fluxConfigurationForm.h": ".+fluxConfigurationForm_h",
        "fluxvisualiserwidget.h": "./fluxvisualiserwidget.h",
    },
)

cxxprogram(
    name="gui2",
    srcs=[
        "./main.cc",
        "./mainwindow.cc",
        "./drivecomponent.cc",
        "./formatcomponent.cc",
        "./fluxvisualiserwidget.cc",
        "./globals.h",
        "./mainwindow.h",
        "./drivecomponent.h",
        "./formatcomponent.h",
    ],
    cflags=["-fPIC"],
    ldflags=["$(QT5_EXTRA_LIBS)"],
    deps=[
        "+fl2_proto_lib",
        "+fmt_lib",
        "+lib",
        "+protobuf_lib",
        "+protocol",
        "+z_lib",
        ".+Qt5Concurrent",
        ".+Qt5Widgets",
        ".+userinterface",
        "dep/adflib",
        "dep/fatfs",
        "dep/hfsutils",
        "dep/libusbp",
        "dep/verdigris",
        "extras+icons",
        "lib+config_proto_lib",
        "src/formats",
        "src/gui/drivetypes",
    ],
)

if config.osx:
    normalrule(
        name="fluxengine_pkg",
        ins=[".+fluxengine_app"],
        outs=["FluxEngine.pkg"],
        commands=[
            "pkgbuild --quiet --install-location /Applications --component {ins[0]} {outs[0]}"
        ],
        label="PKGBUILD",
    )

    normalrule(
        name="fluxengine_app",
        ins=[
            ".+gui",
            "extras+fluxengine_icns",
            "extras/FluxEngine.app.template/",
        ],
        outs=["FluxEngine.app"],
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
