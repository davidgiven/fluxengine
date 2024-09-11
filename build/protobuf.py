from build.ab import Rule, Targets, emit, simplerule, filenamesof
from build.utils import filenamesmatchingof, collectattrs
from types import SimpleNamespace
from os.path import join
import build.pkg  # to get the protobuf package check

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
    simplerule(
        replaces=self,
        ins=srcs,
        outs=[f"={name}.descriptor"],
        deps=deps,
        commands=[
            "$(PROTOC) --include_source_info --descriptor_set_out={outs[0]} {ins}"
        ],
        label="PROTO",
        args={"protosrcs": filenamesof(srcs)},
    )


@Rule
def protocc(self, name, srcs: Targets = [], deps: Targets = []):
    outs = []
    protos = []

    allsrcs = collectattrs(targets=srcs, name="protosrcs")
    assert allsrcs, "no sources provided"
    for f in filenamesmatchingof(allsrcs, "*.proto"):
        cc = f.replace(".proto", ".pb.cc")
        h = f.replace(".proto", ".pb.h")
        protos += [f]
        srcs += [f]
        outs += ["=" + cc, "=" + h]

    r = simplerule(
        name=f"{self.localname}_srcs",
        cwd=self.cwd,
        ins=protos,
        outs=outs,
        deps=deps,
        commands=["$(PROTOC) --cpp_out={dir} {ins}"],
        label="PROTOCC",
    )

    headers = {f[1:]: join(r.dir, f[1:]) for f in outs if f.endswith(".pb.h")}

    from build.c import cxxlibrary

    cxxlibrary(
        replaces=self,
        srcs=[r],
        deps=deps,
        hdrs=headers,
    )


@Rule
def protojava(self, name, srcs: Targets = [], deps: Targets = []):
    outs = []

    allsrcs = collectattrs(targets=srcs, name="protosrcs")
    assert allsrcs, "no sources provided"
    protos = []
    for f in filenamesmatchingof(allsrcs, "*.proto"):
        protos += [f]
        srcs += [f]

    r = simplerule(
        name=f"{self.localname}_srcs",
        cwd=self.cwd,
        ins=protos,
        outs=[f"={self.localname}.srcjar"],
        deps=deps,
        commands=[
            "mkdir -p {dir}/srcs",
            "$(PROTOC) --java_out={dir}/srcs {ins}",
            "$(JAR) cf {outs[0]} -C {dir}/srcs .",
        ],
        traits={"srcjar"},
        label="PROTOJAVA",
    )

    from build.java import javalibrary

    javalibrary(
        replaces=self,
        deps=[r] + deps,
    )
