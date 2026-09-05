Building the client
=====================

The client software is where the intelligence, such as it is, is. It doesn't need the FluxEngine hardware, and will work
either with it, a Greaseweazle, or (to a limited extent) an Applesauce.

It's all written in Java and built with bazel and should work anywhere where bazel is available. Dependencies are pulled
at build time from maven (standard bazel techniques for doing off-line builds apply).

To build it:

- Get [bazelisk](https://github.com/bazelbuild/bazelisk), which is a small tool which wraps bazel
- Get a JDK
- Run `bazelisk build //:fluxengine`

You should end up with an executable in `bazel-bin/java/com/cowlark/fluxengine/fluxengine`. However, this does require
the associated jar file and so isn't easy to install anywhere. You should probably do one of these instead:

- `bazelisk build //:fluxengine_deb` (for Debian/Ubuntu systems)
- `bazelisk build //:fluxengine_rpm` (for Red Hat/Fedora systems)
- `bazelisk build //:fluxengine_app_image` (for generic Linux systems) (this isn't an AppImage binary, it's a Java app
  image, which is a `tar.xz` file you can just unzip somewhere and run)
- `bazelisk build //:fluxengine_dmg` (for OSX systems)
- `bazelisk build //:fluxengine_msi` (for Windows systems)

After building, it'll tell you where the resulting installer lives. You can't cross compile; you'll need to run this on
the platform you're intending to build for.

If you're doing development, you can also do this:

- `bazelisk run //:fluxengine -- $ARGUMENTS`

...which will build and then run the program. Replace `$ARGUMENTS` with the arguments to pass to the program. Use `gui`
to run the GUI.

If it doesn't build, please [get in
touch](https://github.com/davidgiven/fluxengine/issues/new).
