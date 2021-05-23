#include "globals.h"
#include "flags.h"
#include "usb.h"
#include "bytes.h"
#include "fmt/format.h"
#include "usbserial.h"
#include "proto.h"
#include "lib/usb/usb.pb.h"
#include <libusb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined(__linux__)
	#include <libudev.h>
	static int open_serial_device(libusb_device_descriptor* descriptor)
	{
		struct udev* udev = udev_new();
		if (!udev)
			Error() << "udev not available";

		const char* devicenode = NULL;
		struct udev_enumerate* e = udev_enumerate_new(udev);
	    udev_enumerate_add_match_subsystem(e, "tty");
		udev_enumerate_scan_devices(e);
		struct udev_list_entry* devices = udev_enumerate_get_list_entry(e);
		struct udev_list_entry* deve;
		udev_list_entry_foreach(deve, devices)
		{
			const char* path = udev_list_entry_get_name(deve);

			struct udev_device* dev = udev_device_new_from_syspath(udev, path);
			udev_device_ref(dev);
			struct udev_device* parentdev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
			if (parentdev) {
				uint16_t vid = std::stoi(udev_device_get_sysattr_value(parentdev, "idVendor"), 0, 16);
				uint16_t pid = std::stoi(udev_device_get_sysattr_value(parentdev, "idProduct"), 0, 16);
				if ((pid == descriptor->idProduct) && (vid == descriptor->idVendor))
					devicenode = udev_device_get_devnode(dev);
			}
			udev_device_unref(dev);
		}
		udev_enumerate_unref(e);
		udev_unref(udev);

		if (!devicenode)
			return -1;

		int fd = open(devicenode, O_RDWR);
		if (fd == -1)
			Error() << fmt::format("unable to open serial device {}: {}", devicenode, strerror(errno));
		return fd;
	}
#elif defined(WIN32)
	static int open_serial_device(libusb_device_handle* handle)
	{
		return -1;
	}
#elif defined(__APPLE__)
	static int open_serial_device(libusb_device_handle* handle)
	{
		return -1;
	}
#endif

int openUsbSerialDevice(libusb_device_descriptor* descriptor)
{
	if (!config.usb().has_serial())
		return open_serial_device(descriptor);

	std::string path = config.usb().serial();
	int fd = open(path.c_str(), O_RDWR);
	if (fd == -1)
		Error() << fmt::format("unable to open serial device {}: {}", path, strerror(errno));
	return fd;
}

