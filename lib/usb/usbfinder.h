#ifndef USBSERIAL_H
#define USBSERIAL_H

#include <libusb.h>

struct CandidateDevice
{
	libusb_device* device;
	libusb_device_descriptor desc;
	uint32_t id;
	std::string serial;
};

extern std::vector<std::unique_ptr<CandidateDevice>> findUsbDevices(uint32_t id);

#endif

