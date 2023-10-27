from build.ab import normalrule, simplerule
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

normalrule(
    name="fluxengine_iconset",
    ins=["./icon.png"],
    outs=["fluxengine.iconset"],
    commands=[
        "mkdir -p {outs[0]}",
        "sips -z 64 64 {ins[0]} --out {outs[0]}/icon_32x32@2x.png > /dev/null",
    ],
    label="ICONSET",
)

normalrule(
    name="fluxengine_icns",
    ins=[".+fluxengine_iconset"],
    outs=["fluxengine.icns"],
    commands=["iconutil -c icns -o {outs[0]} {ins[0]}"],
    label="ICONUTIL",
)

normalrule(
    name="fluxengine_ico",
    ins=["./icon.png"],
    outs=["fluxengine.ico"],
    commands=[
        "convert {ins[0]} -resize 64x46 -define icon:auto-resize=64,48,32,16 {outs[0]}"
    ],
    label="MAKEICON",
)
