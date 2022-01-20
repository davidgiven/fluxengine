# Platform notes

This page documents some notable differences between the platforms that libusbp supports that can affect the library's behavior.

## Permissions

On Linux, a udev rule is typically needed in order to grant permissions for non-root users to directly access USB devices.  The simplest way to do this would be to add a file named `usb.rules` in the directory `/etc/udev/rules.d` with the following contents, which grants all users permission to access all USB devices:

    SUBSYSTEM=="usb", MODE="0666"


## Multiple handles to the same generic interface

On Linux, you can have multiple simultaneous handles open to the same generic interface of a device.  On Windows and macOS, this is not possible, and you will get an error with the code `LIBUSBP_ERROR_ACCESS_DENIED` when trying to open the second handle with `libusbp_generic_handle_open`.


## Timeouts for interrupt IN endpoints

On macOS, you cannot specify a timeout for an interrupt IN endpoint.  Doing so will result in the following error when you try to read from the endpoint:

    Failed to read from pipe.  (iokit/common) invalid argument.  Error code 0xe00002c2.


## Serial numbers

On Windows, calling `libusbp_device_get_serial_number` on a device that does not actually have a serial number will retrieve some other type of identifier that contains andpersands (&) and numbers.  On other platforms, an error will be returned with the code `LIBUSBP_ERROR_NO_SERIAL_NUMBER`.


## Synchronous operations and Ctrl+C

On Linux, a synchronous (blocking) USB transfer cannot be interrupted by pressing Ctrl+C.  Other signals will probably not work either.


## Interface-specific control transfers

Performing control transfers that are directed to a specific interface might not work correctly on all systems, especially if the device is a composite device and the interface you are connected to is not the one addressed in your control transfer.  For example, see [this message](https://sourceforge.net/p/libusb/mailman/message/34414447/) from the libusb mailing list.
