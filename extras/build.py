from build.ab import simplerule, simplerule
from build.utils import objectify
from build.c import clibrary

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

simplerule(
    name="fluxengine_iconset",
    ins=["./icon.png"],
    outs=["=fluxengine.iconset"],
    commands=[
        "mkdir -p {outs[0]}",
        "sips -z 64 64 {ins[0]} --out {outs[0]}/icon_32x32@2x.png > /dev/null",
    ],
    label="ICONSET",
)

simplerule(
    name="fluxengine_icns",
    ins=[".+fluxengine_iconset"],
    outs=["=fluxengine.icns"],
    commands=["iconutil -c icns -o {outs[0]} {ins[0]}"],
    label="ICONUTIL",
)

simplerule(
    name="fluxengine_ico",
    ins=["./icon.png"],
    outs=["=fluxengine.ico"],
    commands=["png2ico {outs[0]} {ins[0]}"],
    label="MAKEICON",
)
