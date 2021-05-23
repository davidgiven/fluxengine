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

int openUsbSerialDevice(libusb_device_handle* handle)
{
	if (!config.usb().has_serial())
		return -1;

	std::string path = config.usb().serial();
	int fd = open(path.c_str(), O_RDWR);
	if (fd == -1)
		Error() << fmt::format("unable to open serial device {}: {}", path, strerror(errno));
	return fd;
}

