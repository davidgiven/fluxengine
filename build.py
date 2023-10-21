from build.ab import export
from build.c import clibrary
from build.protobuf import proto, protocc

clibrary(name="protocol", hdrs={"protocol.h": "./protocol.h"})

proto(name="fl2_proto", srcs=["lib/fl2.proto"])
protocc(name="fl2_proto_lib", srcs=["+fl2_proto"])

export(name="all", items={"fluxengine": "src+fluxengine"}, deps=["+protocol"])
