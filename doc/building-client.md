Building the client
=====================

The client software is where the intelligence, such as it is, is. It doesn't need the FluxEngine hardware, and will work
either with it, a Greaseweazle, or (to a limited extent) an Applesauce.

The CLI is pretty generic libusb stuff and should build and run on pretty much anything; the GUI is based on ImHex and
will work wherever ImHex does. Support platforms for the FluxEngine client include Windows, Linux and OSX as
well, although on Windows it'll need MSYS2 and mingw32. You'll need to
install some support packages. **You will need to check out git with submodules enabled.**

  - For Linux with Ubuntu/Debian:
    `libudev-dev` `libsqlite3-dev` `protobuf-compiler` `libfmt-dev`
    `libprotobuf-dev` `libmagic-dev` `libmbedtls-dev` `libcurl4-openssl-dev`
    `libmagic-dev` `libdbus-1-dev` `libglfw3-dev` `libfreetype-dev`
    `libboost-regex-dev` Plus, optionally (if not present, internal versions
    will be used): `nlohmann-json3-dev` `libcli11-dev` `libmd4c-dev`
  - For OSX with Homebrew:
    `sqlite` `pkg-config` `libusb` `protobuf` `fmt` `make` `coreutils`
    `dylibbundler` `libjpeg` `libmagic`  `boost` `glfw3` `ninja` `python`
    `freetype2` `mbedtls` Plus, optionally (if not present, internal versions
    will be used): `nlohmann-json` `cli11` `md4c` `lunasvg`
  - For Windows with MSYS and pacboy:
    `protobuf:p` `pkgconf:p` `curl-winssl:p` `file:p` `glfw:p` `mbedtls:p`
    `sqlite:p` `freetype:p` `boost:p` `gcc:p` `binutils:p` `nsis:p`
    `abseil-cpp:p`

There may be mistakes here --- please [get in
touch](https://github.com/davidgiven/fluxengine/issues/new) if I've missed
anything.

Windows and Linux (and other Unixes) build by just doing `make`. OSX builds by
doing `gmake` (we're using a feature which the elderly default make in OSX
doesn't have). Parallel builds will be detected automatically; do `-j1` if for some
reason you don't want one.
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
