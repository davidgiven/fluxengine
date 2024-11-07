# libusbp: Pololu USB Library

[www.pololu.com](https://www.pololu.com/)

The **Pololu USB Library** (also known as **libusbp**) is a cross-platform C library for accessing USB devices.

## Features

- Can retrieve the vendor ID, product ID, revision, and serial number for each connected USB device.
- Can perform I/O on generic (vendor-defined) USB interfaces:
  - Synchronous control transfers.
  - Synchronous and asynchronous bulk/interrupt transfers on IN endpoints.
  - Synchronous bulk/interrupt transfers on OUT endpoints.
- Can retrieve the names of virtual serial ports provided by a specified USB device (e.g. "COM5").
- Provides detailed error information to the caller.
  - Each error includes one or more English sentences describing the error, including error codes from underlying APIs.
  - Some errors have libusbp-defined error codes that can be used to programmatically decide how to handle the error.
- Provides an object-oriented C++ wrapper.
- Provides access to underlying identifiers, handles, and file descriptors.


## Supported platforms

The library runs on:

* Microsoft Windows (Windows Vista and later)
* Linux
* macOS (10.11 and later)


## Supported USB devices and interfaces

The library only supports certain types of USB devices.

On Windows, any generic interface that you want to access with this library must use WinUSB as its driver, so you will need to provide a driver package or use some other mechanism to make sure WinUSB is installed.  The generic interface must have a registry key named "DeviceInterfaceGUIDs" which is a REG_MULTI_SZ, and the first string in the key must be a valid GUID.  The "DeviceInterfaceGUID" key is not supported.

Both composite and non-composite devices are supported.

There is no support for switching device configurations or switching between alternate settings of an interface.

There is no support for accessing a generic interface that is included in an interface association and is not the first interface in the association.


## Platform differences

This library is a relatively simple wrapper around low-level USB APIs provided by each supported platform.  Because these APIs have different capabilities and behavior, you should not assume your program will work on a platform on which it has not been tested.  Some notable differences are documented in `PLATFORM_NOTES.md`.


## Comparison to libusb

This library has a lot in common with [libusb 1.0](http://libusb.info/), which has been around for longer and has many more features.  However, libusbp does have some useful features that libusb lacks, such as listing the serial number of a USB device without performing I/O or getting information about a USB device's virtual serial ports.


## Building from source on Windows with MSYS2

The recommended way to build this library on Windows is to use [MSYS2](http://msys2.github.io/).

After installing MSYS2, launch it by selecting "MSYS2 MinGW 32-bit" or "MSYS2 MinGW 64-bit" from the Start Menu.  Then run this command to install the required packages:

    pacman -S $MINGW_PACKAGE_PREFIX-{toolchain,cmake}

If pacman prompts you to enter a selection of packages to install, just press enter to install all of the packages.

Download the source code of this library and navigate to the top-level directory using `cd`.  Then run these commands to build the library and install it:

    mkdir build
    cd build
    MSYS2_ARG_CONV_EXCL=- cmake .. -G"MSYS Makefiles" -DCMAKE_INSTALL_PREFIX=$MINGW_PREFIX
    make install DESTDIR=/

We currently do not provide any build files for Visual Studio.  You can use CMake to generate Visual Studio build files, and the library and its examples will probably compile, but we have not tested the resulting library.


## Building from source on Linux

First, you will need to make sure that you have a suitable compiler installed, such as gcc.  You can run `gcc -v` in a shell to make sure it is available.

You will also need to install CMake and libudev.  On Ubuntu, Raspbian, and other Debian-based distributions, the command to do this is:

    sudo apt-get install cmake libudev-dev

On Arch Linux, libudev should already be installed, and you can install CMake by running:

    sudo pacman -S cmake

Download the source code of this library and navigate to the top-level directory using `cd`.  Then run these commands to build the library and install it:

    mkdir build
    cd build
    cmake ..
    make
    sudo make install


## Building from source on macOS

First, install [Homebrew](http://brew.sh/), a package manager for macOS.  Then use Homebrew to install CMake by running the following command in a Terminal:

    brew install cmake

Download the source code of this library and navigate to the top-level directory using `cd`.  Then run these commands in a Terminal to build the library and install it:

    mkdir build
    cd build
    cmake ..
    make
    sudo make install


## Incorporating libusbp into a C/C++ project

The first step to incorporating libusbp into another project is to add the libusbp header folder to your project's include search path.
The header folder is the folder that contains `libusbp.h` and `libusbp.hpp`, and it will typically be named `libusbp-1`.
On systems with `pkg-config`, assuming libusbp has been installed properly, you can run the following command to get the include directory:

    pkg-config --cflags libusbp-1

The output of that command is formatted so that it can be directly added to the command-line arguments for most compilers.

Next, you should include the appropriate libusbp header in your source code by adding an include directive at the top of one of your source files.
To use the C API from a C or C++ project, you should write:

    #include <libusbp.h>

To use the C++ API from a C++ project, you should write:

    #include <libusbp.hpp>

After making the changes above, you should be able compile your project successfully.
If the compiler says that the libusbp header file cannot be found, make sure you have specified the include path correctly as described above.

If you add a call to any of the libusbp functions and rebuild, you will probably get an undefined reference error from the linker.  To fix this, you need to add libusbp's linker settings to your project.  To get the linker settings, run:

    pkg-config --libs libusbp-1

If you are using GCC and a shell that supports Bash-like syntax, here is an example command that compiles a single-file C program using the correct compiler and linker settings:

    gcc program.c `pkg-config --cflags --libs libusbp-1`

Here is an equivalent command for C++:

    g++ program.cpp `pkg-config --cflags --libs libusbp-1`

The order of the arguments above matters: the user program must come before libusbp because it relies on symbols that are defined by libusbp.

The `examples` folder of this repository contains some example code that uses libusbp.  These examples can serve as a starting point for your own project.


## Versioning

The version numbers used by this library follow the rules of [Semantic Versioning 2.0.0](http://semver.org/).

A backwards-incompatible version of this library might be released in the future.
If that happens, the new version will have a different major version number: its version number will be 2.0.0 and it will be known as libusbp-2 to `pkg-config`.
This library was designed to support having different major versions installed side-by-side on the same computer, so you could have both libusbp-1 and libusbp-2 installed at the same time.
However, you would not be able to use both versions from a single program.

If you write software that depends on libusbp, we recommend specifying which major version of libusbp your software uses in both the documentation of your software and in the scripts used to build it.  Scripts or instructions for downloading the source code of libusbp should use a branch name to ensure that they downlod the latest version of the code for a given major version number.  For example, the branch of this repository named "v1-latest" will always point to the latest release of libusbp-1.


## Documentation

For detailed documentation of this library, see the header files `libusb.h` and `libusbp.hpp` in the `include` directory, and see the `.md` files in this directory.  You can also use [Doxygen](http://doxygen.org/) to generate documentation from those files.


## Version history

* 1.3.0 (2023-01-02):
  * Windows: Added support for detecting FTDI serial ports.  (FTDI devices with more than one port have not been tested and the interface for detecting them might change in the future.)
  * macOS: Fixed the detection of serial ports for devices that are not CDC ACM.
* 1.2.0 (2020-11-16):
  * Linux: Made the library work with devices attached to the cp210x driver.
  * Windows: Made the library work with devices that have lowercase letters in their hardware IDs.
* 1.1.0 (2018-11-23):
  * Added `libusbp_write_pipe`.
* 1.0.4 (2017-08-29):
  * Fixed a compilation error for macOS.
  * Added the `lsport` example for listing multiple USB serial ports.
* 1.0.3 (2017-08-29):
  * Compiler flags from libudev.pc are now used.
  * For static builds, libusbp-1.pc now requires libudev.pc instead of copying its flags.
  * Always build the install helper as a shared library (DLL).
* 1.0.2 (2017-06-29):
  * Fixed some issues with using libusbp as a static library.
  * Added the `drop_in` example, which shows how to use libusbp as a three-file library.
* 1.0.1 (2016-03-20): Fixed the `libusbp_broadcast_setting_change*` functions (a bonus feature for the Windows version).
* 1.0.0 (2016-03-02): Original release.
