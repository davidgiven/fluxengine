from os.path import basename, join
from build.ab import (
    ABException,
    Rule,
    Targets,
    TargetsMap,
    filenameof,
    flatten,
    filenamesof,
    normalrule,
    stripext,
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
        cflags=cflags,
    )


@Rule
def cfile(
    self,
    name,
    srcs: Targets = [],
    deps: Targets = [],
    cflags=[],
    suffix=".o",
    commands=["$(CC) -c -o {outs[0]} {ins[0]} $(CFLAGS) {cflags}"],
    label="CC",
):
    cfileimpl(self, name, srcs, deps, suffix, commands, label, "cfile", cflags)


@Rule
def cxxfile(
    self,
    name,
    srcs: Targets = [],
    deps: Targets = [],
    cflags=[],
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
            if f.endswith(".c") or f.endswith(".cc") or f.endswith(".cpp")
        ]

    return objs


def libraryimpl(
    self, name, srcs, deps, hdrs, cflags, ldflags, commands, label, kind
):
    if not srcs and not hdrs:
        raise ABException(
            "clibrary contains no sources and no exported headers"
        )

    libraries = [d for d in deps if hasattr(d, "clibrary")]
    for library in libraries:
        if library.clibrary.cflags:
            cflags += library.clibrary.cflags
        if library.clibrary.ldflags:
            ldflags += library.clibrary.ldflags

    for f in filenamesof(srcs):
        if f.endswith(".h"):
            deps += [f]

    hdrcs = []
    hdrins = list(hdrs.values())
    hdrouts = []
    i = 0
    for dest, src in hdrs.items():
        s = filenamesof(src)
        if len(s) != 1:
            raise ABException(
                "a dependency of an export must have exactly one output file"
            )

        hdrcs += ["cp {ins[" + str(i) + "]} {outs[" + str(i) + "]}"]
        hdrouts += [dest]
        i = i + 1

    if not hasattr(self, "clibrary"):
        self.clibrary = SimpleNamespace()
    if srcs:
        hr = None
        if hdrcs:
            hr = normalrule(
                name=f"{name}_hdrs",
                ins=hdrins,
                outs=hdrouts,
                label="HEADERS",
                commands=hdrcs,
            )
            hr.materialise()

        actualsrcs = findsources(
            name,
            srcs,
            deps + ([f"{name}_hdrs"] if hr else []),
            cflags + ([f"-I{hr.normalrule.objdir}"] if hr else []),
            kind,
        )

        normalrule(
            replaces=self,
            ins=actualsrcs,
            outs=[basename(name) + ".a"],
            label=label,
            commands=commands if actualsrcs else [],
        )

        self.clibrary.ldflags = ldflags
        self.clibrary.cflags = ["-I" + hr.normalrule.objdir] if hr else []
    else:
        r = normalrule(
            replaces=self,
            ins=hdrins,
            outs=hdrouts,
            label="HEADERS",
            commands=hdrcs,
        )
        r.materialise()

        self.clibrary.ldflags = ldflags
        self.clibrary.cflags = ["-I" + r.normalrule.objdir]


@Rule
def clibrary(
    self,
    name,
    srcs: Targets = [],
    deps: Targets = [],
    hdrs: TargetsMap = {},
    cflags=[],
    ldflags=[],
    commands=["$(AR) cqs {outs[0]} {ins}"],
    label="LIB",
):
    return libraryimpl(
        self, name, srcs, deps, hdrs, cflags, ldflags, commands, label, cfile
    )


@Rule
def cxxlibrary(
    self,
    name,
    srcs: Targets = [],
    deps: Targets = [],
    hdrs: TargetsMap = {},
    cflags=[],
    ldflags=[],
    commands=["$(AR) cqs {outs[0]} {ins}"],
    label="LIB",
):
    return libraryimpl(
        self, name, srcs, deps, hdrs, cflags, ldflags, commands, label, cxxfile
    )


def programimpl(
    self, name, srcs, deps, cflags, ldflags, commands, label, filerule, kind
):
    libraries = [d for d in deps if hasattr(d, "clibrary")]
    for library in libraries:
        if library.clibrary.cflags:
            cflags += library.clibrary.cflags
        if library.clibrary.ldflags:
            ldflags += library.clibrary.ldflags

    deps += [f for f in filenamesof(srcs) if f.endswith(".h")]

    ars = [f for f in filenamesof(libraries) if f.endswith(".a")]
    normalrule(
        replaces=self,
        ins=(findsources(name, srcs, deps, cflags, filerule) + ars + ars),
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
    srcs: Targets = [],
    deps: Targets = [],
    cflags=[],
    ldflags=[],
    commands=["$(CC) -o {outs[0]} {ins} {ldflags} $(LDFLAGS)"],
    label="CLINK",
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
        cfile,
        "cprogram",
    )


@Rule
def cxxprogram(
    self,
    name,
    srcs: Targets = [],
    deps: Targets = [],
    cflags=[],
    ldflags=[],
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
