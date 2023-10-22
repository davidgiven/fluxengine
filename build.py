from build.ab import export
from build.c import clibrary
from build.protobuf import proto, protocc
from build.pkg import package

package(name="protobuf_lib", package="protobuf")
package(name="z_lib", package="zlib")
package(name="fmt_lib", package="fmt")
package(name="sqlite3_lib", package="sqlite3")

clibrary(name="protocol", hdrs={"protocol.h": "./protocol.h"})

proto(name="fl2_proto", srcs=["lib/fl2.proto"])
protocc(name="fl2_proto_lib", srcs=["+fl2_proto"])

export(
    name="all",
    items={
        "fluxengine": "src+fluxengine",
        "fluxengine-gui": "src/gui",
        "brother120tool": "tools+brother120tool",
        "brother240tool": "tools+brother240tool",
        "upgrade-flux-file": "tools+upgrade-flux-file",
    },
    deps=["+protocol"],
)
