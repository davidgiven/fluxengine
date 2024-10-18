from build.ab import Rule, Targets, emit, simplerule, filenamesof
from build.utils import filenamesmatchingof, collectattrs
from os.path import join, abspath, dirname, relpath
import build.pkg  # to get the protobuf package check

emit(
    """
PROTOC ?= protoc
"""
)

assert build.pkg.TargetPkgConfig.has_package(
    "protobuf"
), "required package 'protobuf' not installed"


def _getprotodeps(deps):
    r = set()
    for d in deps:
        r.update(d.args.get("protodeps", {d}))
    return sorted(r)


@Rule
def proto(self, name, srcs: Targets = [], deps: Targets = []):
    protodeps = _getprotodeps(deps)
    descriptorlist = ":".join(
        [
            relpath(f, start=self.dir)
            for f in filenamesmatchingof(protodeps, "*.descriptor")
        ]
    )

    dirs = sorted({"{dir}/" + dirname(f) for f in filenamesof(srcs)})
    simplerule(
        replaces=self,
        ins=srcs,
        outs=[f"={self.localname}.descriptor"],
        deps=protodeps,
        commands=(
            ["mkdir -p " + (" ".join(dirs))]
            + [f"$(CP) {f} {{dir}}/{f}" for f in filenamesof(srcs)]
            + [
                "cd {dir} && "
                + (
                    " ".join(
                        [
                            "$(PROTOC)",
                            "--proto_path=.",
                            "--include_source_info",
                            f"--descriptor_set_out={self.localname}.descriptor",
                        ]
                        + (
                            [f"--descriptor_set_in={descriptorlist}"]
                            if descriptorlist
                            else []
                        )
                        + ["{ins}"]
                    )
                )
            ]
        ),
        label="PROTO",
        args={
            "protosrcs": filenamesof(srcs),
            "protodeps": set(protodeps) | {self},
        },
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
        outs += ["=" + cc, "=" + h]

    protodeps = _getprotodeps(deps + srcs)
    descriptorlist = ":".join(
        [
            relpath(f, start=self.dir)
            for f in filenamesmatchingof(protodeps, "*.descriptor")
        ]
    )

    r = simplerule(
        name=f"{self.localname}_srcs",
        cwd=self.cwd,
        ins=srcs,
        outs=outs,
        deps=protodeps,
        commands=[
            "cd {dir} && "
            + (
                " ".join(
                    [
                        "$(PROTOC)",
                        "--proto_path=.",
                        "--cpp_out=.",
                        f"--descriptor_set_in={descriptorlist}",
                    ]
                    + protos
                )
            )
        ],
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

    descriptorlist = ":".join(
        [abspath(f) for f in filenamesmatchingof(srcs + deps, "*.descriptor")]
    )

    r = simplerule(
        name=f"{self.localname}_srcs",
        cwd=self.cwd,
        ins=protos,
        outs=[f"={self.localname}.srcjar"],
        deps=srcs + deps,
        commands=[
            "mkdir -p {dir}/srcs",
            "cd {dir} && "
            + (
                " ".join(
                    [
                        "$(PROTOC)",
                        "--proto_path=.",
                        "--java_out=.",
                        f"--descriptor_set_in={descriptorlist}",
                    ]
                    + protos
                )
            ),
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
