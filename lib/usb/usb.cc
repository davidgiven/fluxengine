#include "globals.h"
#include "flags.h"
#include "usb.h"
#include "protocol.h"
#include "fluxmap.h"
#include "bytes.h"
#include <libusb.h>
#include "fmt/format.h"
#include "greaseweazle.h"

FlagGroup usbFlags;

static StringFlag device(
    { "--device" },
    "serial number of hardware device to use",
	"");

static USB* usb = NULL;

enum
{
	DEV_FLUXENGINE,
	DEV_GREASEWEAZLE,
};

struct CandidateDevice
{
	libusb_device* device;
	libusb_device_descriptor desc;
	int type;
	std::string serial;
};

USB::~USB()
{}

std::string USB::usberror(int i)
{
    return libusb_strerror((libusb_error) i);
}

static const char* device_type(int i)
{
	switch (i)
	{
		case DEV_FLUXENGINE: return "FluxEngine";
		case DEV_GREASEWEAZLE: return "GreaseWeazle";
		default: assert(false);
	}
	return NULL;
}

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

static std::map<std::string, std::unique_ptr<CandidateDevice>> get_candidates(libusb_device** devices, int numdevices)
{
	std::map<std::string, std::unique_ptr<CandidateDevice>> candidates;
	for (int i=0; i<numdevices; i++)
	{
		std::unique_ptr<CandidateDevice> candidate(new CandidateDevice());
		candidate->device = devices[i];
		(void) libusb_get_device_descriptor(devices[i], &candidate->desc);

		uint32_t id = (candidate->desc.idVendor << 16) | candidate->desc.idProduct;
		switch (id)
		{
			case (FLUXENGINE_VID<<16) | FLUXENGINE_PID:
			{
				candidate->type = DEV_FLUXENGINE;
				candidate->serial = get_serial_number(candidate->device, &candidate->desc);
				candidates[candidate->serial] = std::move(candidate);
				break;
			}

			case (GREASEWEAZLE_VID<<16) | GREASEWEAZLE_PID:
			{
				candidate->type = DEV_GREASEWEAZLE;
				candidate->serial = get_serial_number(candidate->device, &candidate->desc);
				candidates[candidate->serial] = std::move(candidate);
				break;
			}
		}
	}

	return candidates;
}

static void open_device(CandidateDevice& candidate)
{
	libusb_device_handle* handle;
	int i = libusb_open(candidate.device, &handle);
	if (i < 0)
		Error() << "cannot open USB device: " << libusb_strerror((libusb_error) i);
	
	std::cout << "Using " << device_type(candidate.type) << " with serial number " << candidate.serial << '\n';
	switch (candidate.type)
	{
		case DEV_FLUXENGINE:
			usb = createFluxengineUsb(handle);
			break;

		case DEV_GREASEWEAZLE:
			usb = createGreaseWeazleUsb(handle);
			break;
	}
}

static CandidateDevice& select_candidate(const std::map<std::string, std::unique_ptr<CandidateDevice>>& devices)
{
	if (devices.size() == 0)
		Error() << "no USB devices found (is one plugged in? Do you have permission to access USB devices?)";

	if (device.get() == "")
	{
		if (devices.size() == 1)
			return *devices.begin()->second;

		std::cout << "More than one USB device detected. Use --device to specify which one to use:\n";
		for (auto& i : devices)
			std::cout << "  " << device_type(i.second->type) << ": " << i.first << '\n';
		Error() << "specify USB device";
	}
	else
	{
		const auto& i = devices.find(device);
		if (i != devices.end())
			return *i->second;

		Error() << "device with serial number '" << device.get() << "' not found";
	}
}

USB& getUsb()
{
	if (!usb)
	{
		int i = libusb_init(NULL);
		if (i < 0)
			Error() << "could not start libusb: " << libusb_strerror((libusb_error) i);

		libusb_device** devices;
		int numdevices = libusb_get_device_list(NULL, &devices);
		if (numdevices < 0)
			Error() << "could not enumerate USB bus: " << libusb_strerror((libusb_error) numdevices);

		auto candidates = get_candidates(devices, numdevices);
		auto candidate = select_candidate(candidates);
		open_device(candidate);

		libusb_free_device_list(devices, true);

	}

	return *usb;
}

