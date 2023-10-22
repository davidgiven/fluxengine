from collections.abc import Iterable, Sequence
from os.path import *
from types import SimpleNamespace
import argparse
import copy
import functools
import importlib
import importlib.abc
import importlib.util
import inspect
import re
import sys
import types
import pathlib
import builtins
import os

defaultGlobals = {}
targets = {}
unmaterialisedTargets = set()
materialisingStack = []
outputFp = None
cwdStack = [""]

sys.path += ["."]
old_import = builtins.__import__


def new_import(name, *args, **kwargs):
    if name not in sys.modules:
        path = name.replace(".", "/") + ".py"
        if isfile(path):
            sys.stderr.write(f"loading {path}\n")
            loader = importlib.machinery.SourceFileLoader(name, path)

            spec = importlib.util.spec_from_loader(
                name, loader, origin="built-in"
            )
            module = importlib.util.module_from_spec(spec)
            sys.modules[name] = module
            cwdStack.append(dirname(path))
            spec.loader.exec_module(module)
            cwdStack.pop()

    return old_import(name, *args, **kwargs)


builtins.__import__ = new_import


class ABException(BaseException):
    pass


class ParameterList(Sequence):
    def __init__(self, parent=[]):
        self.data = parent

    def __getitem__(self, i):
        return self.data[i]

    def __len__(self):
        return len(self.data)

    def __str__(self):
        return " ".join(self.data)

    def __add__(self, other):
        newdata = self.data.copy() + other
        return ParameterList(newdata)

    def __repr__(self):
        return f"<PList: {self.data}>"


class Invocation:
    name = None
    callback = None
    types = None
    ins = None
    outs = None
    binding = None

    def materialise(self, replacing=False):
        if self in unmaterialisedTargets:
            if not replacing and (self in materialisingStack):
                print("Found dependency cycle:")
                for i in materialisingStack:
                    print(f"  {i.name}")
                print(f"  {self.name}")
                sys.exit(1)

            materialisingStack.append(self)

            # Perform type conversion to the declared rule parameter types.

            try:
                self.args = {}
                for k, v in self.binding.arguments.items():
                    if k != "kwargs":
                        t = self.types.get(k, None)
                        if t:
                            v = t(v).convert(self)
                        self.args[k] = v
                    else:
                        for kk, vv in v.items():
                            t = self.types.get(kk, None)
                            if t:
                                vv = t(vv).convert(self)
                            self.args[kk] = vv

                # Actually call the callback.

                cwdStack.append(self.cwd)
                self.callback(**self.args)
                cwdStack.pop()
            except BaseException as e:
                print(
                    f"Error materialising {self} ({id(self)}): {self.callback}"
                )
                print(f"Arguments: {self.args}")
                raise e

            if self.outs is None:
                raise ABException(f"{self.name} didn't set self.outs")

            if self in unmaterialisedTargets:
                unmaterialisedTargets.remove(self)

            materialisingStack.pop()

    def __repr__(self):
        return "<Invocation %s>" % self.name


def Rule(func):
    sig = inspect.signature(func)

    @functools.wraps(func)
    def wrapper(*, name=None, replaces=None, **kwargs):
        cwd = None
        if name:
            if ("+" in name) and not name.startswith("+"):
                (cwd, _) = name.split("+", 1)
        if not cwd:
            cwd = cwdStack[-1]

        if name:
            i = Invocation()
            if name.startswith("./"):
                name = join(cwd, name)
            elif "+" not in name:
                name = cwd + "+" + name

            i.name = name
            i.localname = name.split("+")[-1]

            if name in targets:
                raise ABException(f"target {i.name} has already been defined")
            targets[name] = i
        elif replaces:
            i = replaces
            name = i.name
        else:
            raise ABException("you must supply either name or replaces")

        i.cwd = cwd
        i.types = func.__annotations__
        i.callback = func
        setattr(i, func.__name__, SimpleNamespace())

        i.binding = sig.bind(name=name, self=i, **kwargs)
        i.binding.apply_defaults()

        unmaterialisedTargets.add(i)
        if replaces:
            i.materialise(replacing=True)
        return i

    defaultGlobals[func.__name__] = wrapper
    return wrapper


class Type:
    def __init__(self, value):
        self.value = value


class Targets(Type):
    def convert(self, invocation):
        value = self.value
        if type(value) is str:
            value = [value]
        if type(value) is list:
            value = targetsof(value, cwd=invocation.cwd)
        return value


class Target(Type):
    def convert(self, invocation):
        value = self.value
        if not value:
            return None
        return targetof(value, cwd=invocation.cwd)


class TargetsMap(Type):
    def convert(self, invocation):
        value = self.value
        if type(value) is dict:
            return {
                k: targetof(v, cwd=invocation.cwd) for k, v in value.items()
            }
        raise ABException(f"wanted a dict of targets, got a {type(value)}")


def flatten(*xs):
    def recurse(xs):
        for x in xs:
            if isinstance(x, Iterable) and not isinstance(x, (str, bytes)):
                yield from recurse(x)
            else:
                yield x

    return list(recurse(xs))


def fileinvocation(s):
    i = Invocation()
    i.name = s
    i.outs = [s]
    targets[s] = i
    return i


def targetof(s, cwd):
    if isinstance(s, Invocation):
        s.materialise()
        return s

    if s in targets:
        t = targets[s]
        t.materialise()
        return t

    if s.startswith(".+"):
        s = cwd + s[1:]
    elif s.startswith("./"):
        s = normpath(join(cwd, s))
    elif s.startswith("$"):
        return fileinvocation(s)

    if "+" not in s:
        if isdir(s):
            s = s + "+" + basename(s)
        else:
            return fileinvocation(s)

    (path, target) = s.split("+", 2)
    loadbuildfile(join(path, "build.py"))
    if not s in targets:
        raise ABException(f"build file at {path} doesn't contain +{target}")
    i = targets[s]
    i.materialise()
    return i


def targetsof(*xs, cwd):
    return flatten([targetof(x, cwd) for x in flatten(xs)])


def filenamesof(*xs):
    s = []
    for t in flatten(xs):
        if type(t) == str:
            t = normpath(t)
            if t not in s:
                s += [t]
        else:
            for f in [normpath(f) for f in filenamesof(t.outs)]:
                if f not in s:
                    s += [f]
    return s


def targetnamesof(*xs):
    s = []
    for x in flatten(xs):
        if type(x) == str:
            x = normpath(x)
            if x not in s:
                s += [x]
        else:
            if x.name not in s:
                s += [x.name]
    return s


def filenameof(x):
    xs = filenamesof(x)
    if len(xs) != 1:
        raise ABException("expected a single item")
    return xs[0]


def stripext(path):
    return splitext(path)[0]


def emit(*args):
    outputFp.write(" ".join(flatten(args)))
    outputFp.write("\n")


def templateexpand(s, invocation):
    class Converter:
        def __getitem__(self, key):
            if key == "self":
                return invocation
            f = filenamesof(invocation.args[key])
            if isinstance(f, Sequence):
                f = ParameterList(f)
            return f

    return eval("f%r" % s, invocation.callback.__globals__, Converter())


def emitter_rule(name, ins, outs, deps=[]):
    emit("")
    emit(".PHONY:", name)
    if outs:
        emit(name, ":", filenamesof(outs), ";")
        emit(filenamesof(outs), "&:", filenamesof(ins), filenamesof(deps))
    else:
        emit(name, "&:", filenamesof(ins), filenamesof(deps))


def emitter_endrule(name):
    pass


def emitter_label(s):
    emit("\t$(hide)", "$(ECHO)", s)


def emitter_exec(cs):
    for c in cs:
        emit("\t$(hide)", c)


def unmake(*ss):
    return [
        re.sub(r"\$\(([^)]*)\)", r"$\1", s) for s in flatten(filenamesof(ss))
    ]


@Rule
def simplerule(
    self,
    name,
    ins: Targets = [],
    outs=[],
    deps: Targets = [],
    commands=[],
    label="RULE",
    **kwargs,
):
    self.ins = ins
    self.outs = outs
    self.deps = deps
    emitter_rule(self.name, ins + deps, outs)
    emitter_label(templateexpand("{label} {name}", self))

    dirs = []
    for out in filenamesof(outs):
        dir = dirname(out)
        if dir and dir not in dirs:
            dirs += [dir]

        cs = [("mkdir -p %s" % dir) for dir in dirs]
    for c in commands:
        cs += [templateexpand(c, self)]
    emitter_exec(cs)
    emitter_endrule(self.name)


@Rule
def normalrule(
    self,
    name=None,
    ins: Targets = [],
    deps: Targets = [],
    outs=[],
    label="RULE",
    objdir=None,
    commands=[],
    **kwargs,
):
    objdir = objdir or join("$(OBJ)", name)

    self.normalrule.objdir = objdir
    simplerule(
        replaces=self,
        ins=ins,
        deps=deps,
        outs=[join(objdir, f) for f in outs],
        label=label,
        commands=commands,
        **kwargs,
    )


@Rule
def export(self, name=None, items: TargetsMap = {}, deps: Targets = []):
    cs = []
    self.ins = items.values()
    self.outs = []
    for dest, src in items.items():
        destf = filenameof(dest)
        dir = dirname(destf)
        if dir:
            cs += ["mkdir -p " + dir]

        srcs = filenamesof(src)
        if len(srcs) != 1:
            raise ABException(
                "a dependency of an export must have exactly one output file"
            )

        cs += ["cp %s %s" % (srcs[0], destf)]
        self.outs += [destf]

    emitter_rule(self.name, items.values(), self.outs, deps)
    emitter_label(f"EXPORT {self.name}")

    emitter_exec(cs)

    if self.outs:
        emit("clean::")
        emit("\t$(hide) rm -f " + (" ".join(filenamesof(self.outs))))
    self.outs += deps

    emitter_endrule(self.name)


def loadbuildfile(filename):
    filename = filename.replace("/", ".").removesuffix(".py")
    builtins.__import__(filename)


def load(filename):
    loadbuildfile(filename)
    callerglobals = inspect.stack()[1][0].f_globals
    for k, v in defaultGlobals.items():
        callerglobals[k] = v


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-o", "--output")
    parser.add_argument("files", nargs="+")
    parser.add_argument("-t", "--targets", action="append")
    args = parser.parse_args()
    if not args.targets:
        raise ABException("no targets supplied")

    global outputFp
    outputFp = open(args.output, "wt")

    for k in ("Rule", "Targets", "load", "filenamesof", "stripext"):
        defaultGlobals[k] = globals()[k]

    global __name__
    sys.modules["build.ab"] = sys.modules[__name__]
    __name__ = "build.ab"

    for f in args.files:
        loadbuildfile(f)

    for t in flatten([a.split(",") for a in args.targets]):
        if t not in targets:
            raise ABException("target %s is not defined" % t)
        targets[t].materialise()
    emit("AB_LOADED = 1\n")


main()
