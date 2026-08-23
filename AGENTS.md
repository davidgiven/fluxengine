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

## Lombok builders

- The flag classes live in `com.cowlark.fluxengine.core.flags` (one class per file: `Flag`,
  `FlagGroup`, `Flags`, `ActionFlag`, `SettableFlag`, `ValueFlag`, `StringFlag`, `IntFlag`,
  `HexIntFlag`, `DoubleFlag`, `BoolFlag`). Construct flag instances with
  `XxxFlag.builder().setGroup(g).setNames(names).setHelpText(h).build()` rather than
  constructors. `@Builder(setterPrefix = "set")` on the private all-args constructor
  generates the `setX` methods (the ctor param is named `helpText` for `setHelpText`). The
  `core/flags` BUILD defines a `lombok_plugin` (`generates_api = True`, wired via
  `plugins`); lombok is also a compile-time `dep` so the `import lombok.Builder;` resolves.
- Lombok doesn't run under Turbine, and generated classes don't reach the header jar, so
  `.bazelrc` sets `--experimental_java_header_compilation=false`.
- Pattern: put `@Builder` on a private all-args constructor. `@Builder.Default` can't supply
  custom defaults on parameters (illegal `= value` syntax, and defaults to 0/null/false),
  so normalize defaults in the constructor body (e.g. `defaultValue != null ? defaultValue :
  ""`). `FlagGroup.addFlag(this)` happens in the base `Flag` constructor, so `build()`
  registers the flag.
- The `names` parameter is annotated `@Singular`, so builders offer `setName("--foo")`
  (one name at a time), `setNames(collection)`, and `clearNames()` — Lombok can't generate a
  varargs setter, and `@SuperBuilder` is unusable here because its auto-generated constructor
  can't run the `addFlag` side-effect, so `@Singular` avoids hand-writing a builder per class.
- `HexIntFlag` extends `ValueFlag<Integer>` directly (not `IntFlag`): two `@Builder`s would
  both generate a static `builder()` and clash via hiding.

## Flags parsing

- Parsing is done by the static `Flags.parse(ImmutableList<String> argv, FlagGroup... groups)` /
  `Flags.parseWithFilenames(ImmutableList<String> argv, Predicate<String> callback,
  FlagGroup... groups)` (both also accept `ImmutableList<FlagGroup>`). It first runs
  `FlagGroup.initialise` over every root group (recursive duplicate-name check
  into a shared `Set`, marking groups initialised), then walks argv and resolves each flag via
  `FlagGroup.findFlag(key)`, which scans the group's own flags then recurses into its parents.
  `Flags.parse` calls `flag.set(value)` and only consumes a space-separated value when
  `useThat && flag.hasArgument()`. `findFlag` is public and overridable so a group can
  intercept/absorb flags (e.g. a config group) before they fall through to its parents.
- `parseWithFilenames` returns `ImmutableList` (Guava). Duplicate flag names throw
  `IllegalStateException`; unknown flags throw `FluxEngineException`.
- `ConfigFlagGroup` (config package) overrides `findFlag` to intercept dotted `--key.subkey=value`
  arguments: it strips the leading `--` and routes them to `ConfigBuilder.set(path, value)`,
  which delegates to `ProtoPath.set(builder, path, value)`. `ProtoPath` resolves the dotted
  path (with optional `field[4]` indices) against the `ConfigProto` builder via
  `com.google.protobuf` reflection, creating intermediate messages and coercing the string
  value (int/uint/long/float/double/bool/enum) as needed. Unknown paths and bad values throw
  `ConfigException`.

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
- Do not commit unless asked.
