#ifndef USBSERIAL_H
#define USBSERIAL_H

class libusb_device_handle;

extern int openUsbSerialDevice(libusb_device_handle* handle);

#endif

