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
