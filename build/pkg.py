from build.ab import Rule, Target, G
import os
import subprocess


class _PkgConfig:
    package_present = set()
    package_properties = {}
    pkgconfig = None

    def __init__(self, cmd):
        assert cmd, "no pkg-config environment variable supplied"
        self.pkgconfig = cmd

        r = subprocess.run(f"{cmd} --list-all", shell=True, capture_output=True)
        ps = r.stdout.decode("utf-8")
        self.package_present = {l.split(" ", 1)[0] for l in ps.splitlines()}

    def has_package(self, name):
        return name in self.package_present

    def get_property(self, name, flag):
        p = f"{name}.{flag}"
        if p not in self.package_properties:
            r = subprocess.run(
                f"{self.pkgconfig} {flag} {name}",
                shell=True,
                capture_output=True,
            )
            self.package_properties[p] = r.stdout.decode("utf-8").strip()
        return self.package_properties[p]


TargetPkgConfig = _PkgConfig(G.PKG_CONFIG)
HostPkgConfig = _PkgConfig(G.HOST_PKG_CONFIG)


def _package(self, name, package, fallback, pkgconfig):
    if pkgconfig.has_package(package):
        cflags = pkgconfig.get_property(package, "--cflags")
        ldflags = pkgconfig.get_property(package, "--libs")

        if cflags:
            self.args["caller_cflags"] = [cflags]
        if ldflags:
            self.args["caller_ldflags"] = [ldflags]
        self.args["clibrary_deps"] = [self]
        self.args["cheader_deps"] = [self]
        self.traits.update({"clibrary", "cxxlibrary"})
        return

    assert fallback, f"Required package '{package}' not installed"

    if "cheader_deps" in fallback.args:
        self.args["cheader_deps"] = fallback.args["cheader_deps"]
    if "clibrary_deps" in fallback.args:
        self.args["clibrary_deps"] = fallback.args["clibrary_deps"]
    if "cheader_files" in fallback.args:
        self.args["cheader_files"] = fallback.args["cheader_files"]
    if "clibrary_files" in fallback.args:
        self.args["clibrary_files"] = fallback.args["clibrary_files"]
    self.ins = fallback.ins
    self.outs = fallback.outs
    self.deps = fallback.deps
    self.traits = fallback.traits


@Rule
def package(self, name, package=None, fallback: Target = None):
    _package(self, name, package, fallback, TargetPkgConfig)


@Rule
def hostpackage(self, name, package=None, fallback: Target = None):
    _package(self, name, package, fallback, HostPkgConfig)


def has_package(name):
    return TargetPkgConfig.has_package(name)


def has_host_package(name):
    return HostPkgConfig.has_package(name)
