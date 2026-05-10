Building the client
=====================

The client software is where the intelligence, such as it is, is. It doesn't need the FluxEngine hardware, and will work
either with it, a Greaseweazle, or (to a limited extent) an Applesauce.

The CLI is pretty generic libusb stuff and should build and run on pretty much anything; the GUI is based on ImHex and
will work wherever ImHex does. Support platforms for the FluxEngine client include Windows, Linux and OSX as
well, although on Windows it'll need MSYS2 and mingw32. You'll need to
install some support packages. **You will need to check out git with submodules enabled.**

  - For Linux, see the dockerfiles in
    [tests/docker](https://github.com/davidgiven/fluxengine/tree/master/tests/docker).
    These are the source of truth for the dependencies and provide a known-good
    build script.
  - For OSX or Homebrew, see the Github CI scripts in
    [.github/workflows/release.yml](https://github.com/davidgiven/fluxengine/blob/master/.github/workflows/release.yml).
    These are a little harder to read (I haven't figured out how to do Windows or
    OSX docker yet).

Windows and Linux (and other Unixes) build by just doing `make`. OSX builds by
doing `gmake` (we're using a feature which the elderly default make in OSX
doesn't have). Parallel builds will be detected automatically.

**Note for system packagers:** During the build, some remote dependencies will
be pulled in automatically, so you'll need a network connection. If you don't
want this to happen, you can vendor the appropriate dependencies by placing
them in the `dep/r` directory. Look at the `gitrepository` lines in
[build/dep/build.py](https://github.com/davidgiven/fluxengine/blob/ab/dep/build.py)
to see what goes where.

You should end up with some executables in the current directory. The command line version
is called `fluxengine` or `fluxengine.exe` depending on your platform,
and the ImHex-based GUI will be `fluxengine-gui` or `fluxengine-gui.exe`.
They have
minimal dependencies and you should be able to put it anywhere. The other
binaries may also be of interest.

Potential issues:

  - Complaints about a missing `libudev` on Windows? Make sure you're using the
  mingw Python rather than the msys Python.

If it doesn't build, please [get in
touch](https://github.com/davidgiven/fluxengine/issues/new).
