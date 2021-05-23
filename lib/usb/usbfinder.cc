#include "globals.h"
#include "flags.h"
#include "usb.h"
#include "bytes.h"
#include "fmt/format.h"
#include "usbfinder.h"
#include <libusb.h>

static const std::string get_serial_number(libusb_device* device, libusb_device_descriptor* desc)
{
	std::string serial;

	libusb_device_handle* handle;
	if (libusb_open(device, &handle) == 0)
	{
		unsigned char buffer[64];
		libusb_get_string_descriptor_ascii(handle, desc->iSerialNumber, buffer, sizeof(buffer));
		serial = (const char*) buffer;
		libusb_close(handle);
	}

	return serial;
}

std::vector<std::unique_ptr<CandidateDevice>> findUsbDevices(uint32_t candidateId)
{
	int i = libusb_init(NULL);
	if (i < 0)
		Error() << "could not start libusb: " << libusb_strerror((libusb_error) i);

	libusb_device** devices;
	int numdevices = libusb_get_device_list(NULL, &devices);
	if (numdevices < 0)
		Error() << "could not enumerate USB bus: " << libusb_strerror((libusb_error) numdevices);

	std::vector<std::unique_ptr<CandidateDevice>> candidates;
	for (int i=0; i<numdevices; i++)
	{
		std::unique_ptr<CandidateDevice> candidate(new CandidateDevice());
		candidate->device = devices[i];
		(void) libusb_get_device_descriptor(candidate->device, &candidate->desc);

		uint32_t id = (candidate->desc.idVendor << 16) | candidate->desc.idProduct;
		if (id == candidateId)
		{
			libusb_ref_device(candidate->device);
			candidate->id = candidateId;
			candidate->serial = get_serial_number(candidate->device, &candidate->desc);
			candidates.push_back(std::move(candidate));
		}
	}

	libusb_free_device_list(devices, true);
	return candidates;
}
