from os.path import basename, join
from build.ab import (
    ABException,
    List,
    Rule,
    Targets,
    TargetsMap,
    filenameof,
    filenamesmatchingof,
    filenamesof,
    flatten,
    normalrule,
    bubbledattrsof,
    stripext,
    targetswithtraitsof,
)
from os.path import *
from types import SimpleNamespace


def cfileimpl(self, name, srcs, deps, suffix, commands, label, kind, cflags):
    outleaf = stripext(basename(filenameof(srcs[0]))) + suffix

    normalrule(
        replaces=self,
        ins=srcs,
        deps=deps,
        outs=[outleaf],
        label=label,
        commands=commands,
        cflags=cflags + bubbledattrsof(deps, "caller_cflags"),
    )


@Rule
def cfile(
    self,
    name,
    srcs: Targets = None,
    deps: Targets = None,
    cflags: List = [],
    suffix=".o",
    commands=["$(CC) -c -o {outs[0]} {ins[0]} $(CFLAGS) {cflags}"],
    label="CC",
):
    cfileimpl(self, name, srcs, deps, suffix, commands, label, "cfile", cflags)


@Rule
def cxxfile(
    self,
    name,
    srcs: Targets = None,
    deps: Targets = None,
    cflags: List = [],
    suffix=".o",
    commands=["$(CXX) -c -o {outs[0]} {ins[0]} $(CFLAGS) {cflags}"],
    label="CXX",
):
    cfileimpl(
        self, name, srcs, deps, suffix, commands, label, "cxxfile", cflags
    )


def findsources(name, srcs, deps, cflags, filerule):
    objs = []
    for s in flatten(srcs):
        objs += [
            filerule(
                name=join(name, f.removeprefix("$(OBJ)/")),
                srcs=[f],
                deps=deps,
                cflags=cflags,
            )
            for f in filenamesof(s)
            if f.endswith(".c")
            or f.endswith(".cc")
            or f.endswith(".cpp")
            or f.endswith(".S")
            or f.endswith(".s")
        ]
        if any(f.endswith(".o") for f in filenamesof(s)):
            objs += [s]

    return objs


@Rule
def cheaders(
    self,
    name,
    hdrs: TargetsMap = None,
    caller_cflags: List = None,
    deps: Targets = None,
):
    cs = []
    ins = list(hdrs.values())
    outs = []
    i = 0
    for dest, src in hdrs.items():
        s = filenamesof(src)
        if len(s) != 1:
            raise ABException(
                "the target of a header must return exactly one file"
            )

        cs += ["cp {ins[" + str(i) + "]} {outs[" + str(i) + "]}"]
        outs += [dest]
        i = i + 1

    r = normalrule(
        replaces=self,
        ins=ins,
        outs=outs,
        commands=cs,
        deps=deps,
        label="CHEADERS",
    )
    r.materialise()
    self.attr.caller_cflags = caller_cflags + ["-I" + r.attr.objdir]
    self.bubbleattr("caller_cflags", deps)


def libraryimpl(
    self,
    name,
    srcs,
    deps,
    hdrs,
    caller_cflags,
    caller_ldflags,
    cflags,
    ldflags,
    commands,
    label,
    kind,
):
    hr = None
    if hdrs and not srcs:
        cheaders(
            replaces=self,
            hdrs=hdrs,
            deps=targetswithtraitsof(deps, "cheaders"),
            caller_cflags=caller_cflags,
        )
        return
    if hdrs:
        hr = cheaders(
            name=self.localname + "_hdrs",
            hdrs=hdrs,
            deps=targetswithtraitsof(deps, "cheaders"),
            caller_cflags=caller_cflags,
        )
        hr.materialise()
        deps = deps + [hr]

    objs = findsources(
        name,
        srcs,
        targetswithtraitsof(deps, "cheaders"),
        cflags + bubbledattrsof(deps, "caller_cflags"),
        kind,
    )

    normalrule(
        replaces=self,
        ins=objs,
        outs=[basename(name) + ".a"],
        label=label,
        commands=commands,
    )
    self.outs = self.outs + (hr.outs if hr else [])

    self.traits.add("cheaders")
    self.attr.caller_ldflags = caller_ldflags
    self.bubbleattr("caller_ldflags", deps)
    self.bubbleattr("caller_cflags", deps)


@Rule
def clibrary(
    self,
    name,
    srcs: Targets = None,
    deps: Targets = None,
    hdrs: TargetsMap = None,
    caller_cflags: List = [],
    caller_ldflags: List = [],
    cflags: List = [],
    ldflags: List = [],
    commands=["$(AR) cqs {outs[0]} {ins}"],
    label="LIB",
    cfilerule=cfile,
):
    libraryimpl(
        self,
        name,
        srcs,
        deps,
        hdrs,
        caller_cflags,
        caller_ldflags,
        cflags,
        ldflags,
        commands,
        label,
        cfilerule,
    )


@Rule
def cxxlibrary(
    self,
    name,
    srcs: Targets = None,
    deps: Targets = None,
    hdrs: TargetsMap = None,
    caller_cflags: List = [],
    caller_ldflags: List = [],
    cflags: List = [],
    ldflags: List = [],
    commands=["$(AR) cqs {outs[0]} {ins}"],
    label="LIB",
):
    libraryimpl(
        self,
        name,
        srcs,
        deps,
        hdrs,
        caller_cflags,
        caller_ldflags,
        cflags,
        ldflags,
        commands,
        label,
        cxxfile,
    )


def programimpl(
    self, name, srcs, deps, cflags, ldflags, commands, label, filerule, kind
):
    ars = filenamesmatchingof(deps, "*.a")
    deps = deps + filenamesmatchingof(srcs, "*.h")
    ldflags = ldflags + bubbledattrsof(deps, "caller_ldflags")

    cfiles = findsources(name, srcs, deps, cflags, filerule)
    normalrule(
        replaces=self,
        ins=cfiles + ars + ars,
        outs=[basename(name) + "$(EXT)"],
        deps=deps,
        label=label,
        commands=commands,
        ldflags=ldflags,
    )


@Rule
def cprogram(
    self,
    name,
    srcs: Targets = None,
    deps: Targets = None,
    cflags: List = [],
    ldflags: List = [],
    commands=["$(CC) -o {outs[0]} {ins} {ldflags} $(LDFLAGS)"],
    label="CLINK",
    cfilerule=cfile,
    cfilekind="cprogram",
):
    programimpl(
        self,
        name,
        srcs,
        deps,
        cflags,
        ldflags,
        commands,
        label,
        cfilerule,
        cfilekind,
    )


@Rule
def cxxprogram(
    self,
    name,
    srcs: Targets = None,
    deps: Targets = None,
    cflags: List = [],
    ldflags: List = [],
    commands=["$(CXX) -o {outs[0]} {ins} {ldflags} $(LDFLAGS)"],
    label="CXXLINK",
):
    programimpl(
        self,
        name,
        srcs,
        deps,
        cflags,
        ldflags,
        commands,
        label,
        cxxfile,
        "cxxprogram",
    )
