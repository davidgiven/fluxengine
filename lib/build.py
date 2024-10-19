from build.protobuf import proto, protocc


proto(name="common_proto", srcs=["./common.proto"])
protocc(
    name="common_proto_lib", srcs=[".+common_proto"], deps=["+protobuf_lib"]
)
