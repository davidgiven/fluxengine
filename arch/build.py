from build.c import cxxlibrary
from build.protobuf import proto, protocc

proto(
    name="arch_proto",
    srcs=[
        "./aeslanier/aeslanier.proto",
        "./agat/agat.proto",
        "./amiga/amiga.proto",
        "./apple2/apple2.proto",
        "./brother/brother.proto",
        "./c64/c64.proto",
        "./f85/f85.proto",
        "./fb100/fb100.proto",
        "./ibm/ibm.proto",
        "./macintosh/macintosh.proto",
        "./micropolis/micropolis.proto",
        "./mx/mx.proto",
        "./northstar/northstar.proto",
        "./rolandd20/rolandd20.proto",
        "./smaky6/smaky6.proto",
        "./tids990/tids990.proto",
        "./victor9k/victor9k.proto",
        "./zilogmcz/zilogmcz.proto",
    ],
)
