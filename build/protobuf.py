from os.path import join
from build.ab import Rule, Targets, emit, normalrule, filenamesof, flatten
from build.c import cxxlibrary
import build.pkg
from types import SimpleNamespace

emit(
    """
PROTOC ?= protoc
ifeq ($(filter protobuf, $(PACKAGES)),)
$(error Required package 'protobuf' not installed.)"
endif
"""
)


@Rule
def proto(self, name, srcs: Targets = [], deps: Targets = []):
    normalrule(
        replaces=self,
        ins=srcs,
        outs=[f"{name}.descriptor"],
        deps=deps,
        commands=[
            "$(PROTOC) --include_source_info --descriptor_set_out={outs[0]} {ins}"
        ],
        label="PROTO",
    )
    self.proto.srcs = filenamesof(srcs) + flatten(
        [s.proto.srcs for s in flatten(deps)]
    )


@Rule
def protocc(self, name, srcs: Targets = [], deps: Targets = []):
    outs = []
    protos = []
    for f in flatten([s.proto.srcs for s in flatten(srcs + deps)]):
        if f.endswith(".proto"):
            cc = f.replace(".proto", ".pb.cc")
            h = f.replace(".proto", ".pb.h")
            protos += [f]
            srcs += [f]
            outs += [cc, h]

    r = normalrule(
        name=f"{name}_srcs",
        ins=protos,
        outs=outs,
        deps=deps,
        commands=["$(PROTOC) --cpp_out={self.normalrule.objdir} {ins}"],
        label="PROTOCC",
    )

    r.materialise()
    headers = {
        f: join(r.normalrule.objdir, f) for f in outs if f.endswith(".pb.h")
    }

    cxxlibrary(
        replaces=self,
        srcs=[f"{name}_srcs"],
        hdrs=headers,
        cflags=[f"-I{r.normalrule.objdir}"],
    )
