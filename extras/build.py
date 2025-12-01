from build.ab import simplerule, simplerule
from build.utils import objectify, glob
from build.c import clibrary
from build.zip import zip
from os.path import *
import config

icons = ["fluxfile", "hardware", "icon", "imagefile"]

clibrary(
    name="icons",
    hdrs={
        f"icons/{n}.h": objectify(
            name=n + "_h", src=f"./{n}.png", symbol=f"icon_{n}_png"
        )
        for n in icons
    },
)

if config.osx:
    simplerule(
        name="fluxengine_icns",
        ins=["./icon.png"],
        outs=["=fluxengine.icns"],
        commands=[
            "mkdir -p fluxengine.iconset",
            "sips -z 64 64 $[ins[0]] --out fluxengine.iconset/icon_32x32@2x.png > /dev/null",
            "iconutil -c icns -o $[outs[0]] fluxengine.iconset",
        ],
        label="ICONSET",
    )

    zip(
        name="fluxengine_template",
        items={
            join("FluxEngine.app", k): join("extras/FluxEngine.app.template", k)
            for k in glob(
                dir="extras/FluxEngine.app.template",
                relative_to="extras/FluxEngine.app.template",
            )
        },
    )

if config.windows:
    simplerule(
        name="fluxengine_ico",
        ins=["./icon.png"],
        outs=["=fluxengine.ico"],
        commands=["png2ico $[outs[0]] $[ins[0]]"],
        label="MAKEICON",
    )
