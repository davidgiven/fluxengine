# Contributing to libusbp

## Development Tools

The following tools are used to help build, test, debug, and document this library:

- USB Test Device A: This is a custom USB device that is used for testing this library.  You must have a device like this plugged in if you want to run the full test suite.  The `test/firmware/wixel` folder contains firmware that can turn a [Pololu Wixel](https://www.pololu.com/product/1337) into a USB Test Device A, and it should be possible to implement it on other USB-capable boards as well.
- [cmake](http://www.cmake.org)
- [catch](https://github.com/philsquared/Catch)
- [Doxygen](http://www.stack.nl/~dimitri/doxygen/)
- Development environments:
  - Windows: [MSYS2](http://msys2.github.io/)
  - macOS: [Homebrew](http://brew.sh/)
- Memory leak checkers:
  - Windows: Install Dr. Memory and run `drmemory -leaks_only ./run_test.exe`
  - Linux: Install Valgrind and run `valgrind ./run_test`
  - macOS:  Run `MallocStackLogging=1 ./run_test -p`, wait for the tests to finish, press Ctrl+Z, run `leaks run_test`, then finally run `fg` to go back to the stopped test process and end it.

## Underlying API documentation

This section attempts to organize the documentation and information about the underlying APIs used by libusbp, which should be useful to anyone reviewing or editing the code.

### Windows APIs

- [WinUSB](https://msdn.microsoft.com/en-us/library/windows/hardware/ff540196)
- [SetupAPI](https://msdn.microsoft.com/en-us/library/cc185682)
- [PnP Configuration Manager Reference](https://msdn.microsoft.com/en-us/library/windows/hardware/ff549717)
- [Standard USB Identifiers](https://msdn.microsoft.com/en-us/library/windows/hardware/ff553356)
- [What characters or bytes are valid in a USB serial number?](https://msdn.microsoft.com/en-us/library/windows/hardware/dn423379#usbsn)
- [INFO: Windows Rundll and Rundll32 Interface](https://support.microsoft.com/en-us/kb/164787)
- [MSI Custom Action Type 17](https://msdn.microsoft.com/en-us/library/aa368076?f=255&MSPPError=-2147217396)

### Linux APIs

- udev
  - [libudev Reference Manual](https://www.kernel.org/pub/linux/utils/kernel/hotplug/libudev/ch01.html)
  - [Kernel documentation of sysfs-bus-usb](https://www.kernel.org/doc/Documentation/ABI/stable/sysfs-bus-usb)
  - [sysfs.c](https://github.com/torvalds/linux/blob/master/drivers/usb/core/sysfs.c)
  - [libudev and Sysfs Tutorial](http://www.signal11.us/oss/udev/)
- USB device node
  - [USB documentation folder in the kernel tree](https://github.com/torvalds/linux/tree/master/Documentation/usb)
    - [USB error code documentation](https://github.com/torvalds/linux/blob/master/Documentation/usb/error-codes.txt)
  - [usbdevice_fs.h](https://github.com/torvalds/linux/blob/master/include/uapi/linux/usbdevice_fs.h)
  - [devio.c](https://github.com/torvalds/linux/blob/master/drivers/usb/core/devio.c)
  - [Linux USB Drivers presentation](http://free-electrons.com/doc/linux-usb.pdf)
  - [USB Drivers chapter from a book](http://lwn.net/images/pdf/LDD3/ch13.pdf)
  - [linux_usbfs.c from libusb](https://github.com/libusb/libusb/blob/master/libusb/os/linux_usbfs.c)
- Error numbers
  - [errno.h](https://github.com/torvalds/linux/blob/master/include/uapi/asm-generic/errno.h)
  - [errno-base.h](https://github.com/torvalds/linux/blob/master/include/uapi/asm-generic/errno-base.h)

### macOS APIs

- [I/O Kit Framework Reference](https://developer.apple.com/library/mac/documentation/Darwin/Reference/IOKit/index.html#//apple_ref/doc/uid/TP30000815)
- [IOKitLib.h Reference](https://developer.apple.com/library/mac/documentation/IOKit/Reference/IOKitLib_header_reference/)
- [Accessing Hardware From Applications](https://developer.apple.com/library/mac/documentation/DeviceDrivers/Conceptual/AccessingHardware)
- [USB Device Interface Guide](https://developer.apple.com/library/mac/documentation/DeviceDrivers/Conceptual/USBBook/)
- [IOUSBInterfaceClass.cpp](https://github.com/opensource-apple/IOUSBFamily/blob/master/IOUSBLib/Classes/IOUSBInterfaceClass.cpp)
- [IOUSBInterfaceUserClient.cpp](https://github.com/opensource-apple/IOUSBFamily/blob/master/IOUSBUserClient/Classes/IOUSBInterfaceUserClient.cpp)
- [darwin_usb.c from libusb](https://github.com/libusb/libusb/blob/master/libusb/os/darwin_usb.c)
- [Mach messaging interface (mach ports)](http://www.gnu.org/software/hurd/gnumach-doc/Messaging-Interface.html)

## Future development

Here are some things we might want to work on in future versions of the library:

- Serial port support.  (Even just listing the serial ports of a USB device would be useful.)
- Human interface device (HID) support.
- Hotplug support: detect when new devices are added and detect when a specific device is removed.
- Stronger guarantees about thread safety.  (This might not require code changes.)
- More options for asynchronous transfers.
- Perhaps use asynchronous operations to implement the synchronous ones, like libusb does.  This would fix some quirks on various platforms: it would allow us to have timeouts for interrupt endpoints on Mac OS X and it would probably allow long synchronous operations on Linux to be interrupted with Ctrl+C.  The main drawback is that it would add complexity.
- Sending data on OUT pipes.
- A device library template to help people who want to make a cross-platform C or C++ library for a USB device based on libusbp.
