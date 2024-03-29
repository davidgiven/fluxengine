from os.path import join
from build.ab import (
    Rule,
    Targets,
    emit,
    normalrule,
    filenamesof,
    filenamesmatchingof,
    bubbledattrsof,
    targetswithtraitsof,
)
from build.c import cxxlibrary
from types import SimpleNamespace
import build.pkg

emit(
    """
PROTOC ?= protoc
ifeq ($(filter protobuf, $(PACKAGES)),)
$(error Required package 'protobuf' not installed.)"
endif
"""
)


@Rule
def proto(self, name, srcs: Targets = None, deps: Targets = None):
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
    self.attr.protosrcs = filenamesof(srcs)
    self.bubbleattr("protosrcs", deps)


@Rule
def protocc(self, name, srcs: Targets = None, deps: Targets = None):
    outs = []
    protos = []

    for f in filenamesmatchingof(bubbledattrsof(srcs, "protosrcs"), "*.proto"):
        cc = f.replace(".proto", ".pb.cc")
        h = f.replace(".proto", ".pb.h")
        protos += [f]
        srcs += [f]
        outs += [cc, h]

    srcname = f"{name}_srcs"
    objdir = join("$(OBJ)", srcname)
    r = normalrule(
        name=srcname,
        ins=protos,
        outs=outs,
        deps=deps,
        commands=["$(PROTOC) --cpp_out={self.attr.objdir} {ins}"],
        label="PROTOCC",
    )

    headers = {f: join(objdir, f) for f in outs if f.endswith(".pb.h")}

    cxxlibrary(
        replaces=self,
        srcs=[r],
        deps=targetswithtraitsof(deps, "cheaders"),
        hdrs=headers,
    )
