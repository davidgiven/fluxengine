# AGENTS.md

This repo is FluxEngine, a USB floppy-disk drive tool. The existing codebase is C++,
and there is an active, incremental migration of components to Java. The Java side is
the current focus of development. This document describes the Java build structure and
the coding conventions used. Follow it when making changes.

## Build system

Bazel with bzlmod. There is **no WORKSPACE file** — all dependency declarations live in
`MODULE.bazel` (rules_java, rules_jvm_external for Maven deps, rules_proto).

- Java sources: `java/` (standard Bazel layout, `com` is a direct child of `java`)
- Java tests: `javatests/`
- Packages (Java): `com.cowlark.fluxengine` (Main, FluxEngineComponent),
  `com.cowlark.fluxengine.cli`, `com.cowlark.fluxengine.core`, `com.cowlark.fluxengine.core.flags`,
  `com.cowlark.fluxengine.data`, `com.cowlark.fluxengine.usb`, `com.cowlark.fluxengine.wiring`
- Each package directory has its own `BUILD.bazel`.

Useful commands:

- `bazel build //java/...`
- `bazel test //javatests/...`
- `bazel run //java/com/cowlark/fluxengine:fluxengine -- <args>` (JVM binary)
- `bazel build //:fluxengine_deb //:fluxengine_rpm` (jpackage .deb/.rpm installers; root
  aliases `//:fluxengine`, `//:fluxengine_deb`, and `//:fluxengine_rpm` exist)
- `bazel build //:fluxengine_app_image` (jpackage app-image, produced as a tar file)
- `bazel build //:fluxengine_msi //:fluxengine_dmg` (Windows MSI / macOS DMG installers,
  only buildable on their native platforms)

## Gotchas

- Because there is no WORKSPACE, Java rules are **not autoloaded**. Every BUILD file must
  explicitly load what it uses, e.g.
  `load("@rules_java//java:defs.bzl", "java_library", "java_binary", "java_plugin", "java_test")`.
- `javax.usb.properties` must sit at the **classpath root** (the usb4java `Services`
  constructor requires it via `UsbHostManager.getProperties()`). It lives at
  `java/javax.usb.properties`, is exported from `java/BUILD.bazel`, and is pulled in as a
  resource (`resources = ["//java:javax.usb.properties"]`) by the usb library. Bazel's
  resource jarring strips the leading `java/`, so it lands at the jar root. Do not move it
  into the package directory.
- The `.deb` and `.rpm` installers are built with jpackage via the `jpackage` rule in
  `jpackage.bzl` (which uses the configured Java toolchain's `jpackage`). Because `rpmbuild`
  writes to `/var/tmp` and read-only sandbox paths by default, the rule stages everything
  under a writable `workdir/` and, for rpm, points rpmbuild's `_tmppath`/`_builddir` etc. at
  it via a `~/.rpmmacros` file. The `jpackage_app_image` rule produces the raw app-image
  directory as a tar file.
- The MSI (`//:fluxengine_msi`) and DMG (`//:fluxengine_dmg`) targets use `select()` to set
  the jpackage `package_type` per platform (`@platforms//os:windows` → `msi`,
  `@platforms//os:osx` → `dmg`); jpackage can't cross-compile, so on any other platform the
  type is `unsupported`, which makes the rule produce an empty target (so `bazel build
  //java/...` still works everywhere).

## CLI

- Commands live in `com.cowlark.fluxengine.cli` and implement the `Command` interface
  (`String getHelp()`, `void run(ImmutableList<String> args)`), receiving the tail of the argv
  array after
  the command name (modelled on `src/fluxengine.cc`'s `command_cb`).
- `Main.main` holds the command/subcommand tables as `ImmutableMap<String,
  Supplier<? extends Command>>`: `COMMANDS` (top level), `ANALYSABLES`, `FLUXFILEABLES`,
  `TESTABLES`. The tables mirror `src/fluxengine.cc`; unported commands map to
  `StubCommand(name, help)`, which prints "not implemented yet".
- Each command carries its own help text, returned by `getHelp()`; `Main.help` prints the
  table by instantiating each command and calling `getHelp()`.
- `Main.dispatch(commands, args)` consumes arguments until it reaches a real command,
  instantiates it via the supplier (`TestDevicesCommand::new`), and calls `run()` with the
  tail. Group commands
  (`analyse`, `fluxfile`, `test`) are `CommandGroup(subcommands, help)` instances, which
  dispatch again on their sub-table and print extended help if nothing matches. Add new
  commands by updating the relevant table.

## USB

- `UsbFinder` (`java/com/cowlark/fluxengine/usb/`) is the Java port of
  `lib/usb/usbfinder.{cc,h}`. It uses usb4java-javax (javax.usb API). `UsbFinder` is
  Dagger-injectable (`@Inject` constructor, instance methods) and `findUsbDevices()`
  returns an `ImmutableList<CandidateDevice>`.
- `DeviceType` is an enum carrying its display name as a property (`getDeviceName()`).
- jSerialComm is available for serial-port access (not yet used).

## Code style

- Explicit types, no `var`.
- Prefer Guava utilities over hand-rolled checks: `Strings.nullToEmpty(...)` instead of
  explicit null checks; use `ImmutableList` for returned collections.
- Prefer `System.out.printf(...)` over `System.out.println(String.format(...))`.
- Tests use JUnit 4 (`@RunWith(JUnit4.class)`, `org.junit.Test`).
- Follow existing patterns in the package you are editing; keep new functionality
  localized to the relevant package.

## Process

- Verify changes with `bazel build //java/...` and `bazel test //javatests/...` (and
  `bazel run` for CLI-visible behaviour) before finishing.
- Do not commit anything to the VCS; the user will handle commits manually.
