from build.ab import Rule, emit, Target, bubbledattrsof, filenamesof
from types import SimpleNamespace
import os
import subprocess

emit(
    """
PKG_CONFIG ?= pkg-config
PACKAGES := $(shell $(PKG_CONFIG) --list-all | cut -d' ' -f1 | sort)

HOST_PKG_CONFIG ?= pkg-config
HOST_PACKAGES := $(shell $(HOST_PKG_CONFIG) --list-all | cut -d' ' -f1 | sort)
"""
)


@Rule
def package(self, name, package=None, fallback: Target = None):
    self.ins = []
    self.outs = []

    emit("ifeq ($(filter %s, $(PACKAGES)),)" % package)
    if fallback:
        emit(
            f"PACKAGE_CFLAGS_{package} :=",
            bubbledattrsof(fallback, "caller_cflags"),
        )
        emit(
            f"PACKAGE_LDFLAGS_{package} := ",
            bubbledattrsof(fallback, "caller_ldflags"),
        )
        self.outs = filenamesof(fallback)
    else:
        emit(f"$(error Required package '{package}' not installed.)")
    emit("else")
    emit(
        f"PACKAGE_CFLAGS_{package} := $(shell $(PKG_CONFIG) --cflags {package})"
    )
    emit(
        f"PACKAGE_LDFLAGS_{package} := $(shell $(PKG_CONFIG) --libs {package})"
    )
    emit("endif")

    self.attr.caller_cflags = [f"$(PACKAGE_CFLAGS_{package})"]
    self.attr.caller_ldflags = [f"$(PACKAGE_LDFLAGS_{package})"]
    self.traits.add("clibrary")
    self.traits.add("cheaders")


@Rule
def hostpackage(self, name, package=None, fallback: Target = None):
    emit("ifeq ($(filter %s, $(HOST_PACKAGES)),)" % package)
    if fallback:
        emit(
            f"HOST_PACKAGE_CFLAGS_{package} :=",
            bubbledattrsof(fallback, "caller_cflags"),
        )
        emit(
            f"HOST_PACKAGE_LDFLAGS_{package} := ",
            bubbledattrsof(fallback, "caller_ldflags"),
        )
        emit(f"HOST_PACKAGE_DEP_{package} := ", fallback.name)
    else:
        emit(f"$(error Required host package '{package}' not installed.)")
    emit("else")
    emit(
        f"HOST_PACKAGE_CFLAGS_{package} := $(shell $(HOST_PKG_CONFIG) --cflags {package})"
    )
    emit(
        f"HOST_PACKAGE_LDFLAGS_{package} := $(shell $(HOST_PKG_CONFIG) --libs {package})"
    )
    emit(f"HOST_PACKAGE_DEP_{package} := ")
    emit("endif")

    self.attr.caller_cflags = [f"$(HOST_PACKAGE_CFLAGS_{package})"]
    self.attr.caller_ldflags = [f"$(HOST_PACKAGE_LDFLAGS_{package})"]

    self.ins = []
    self.outs = [f"$(HOST_PACKAGE_DEP_{package})"]
