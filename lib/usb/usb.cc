#include "globals.h"
#include "flags.h"
#include "usb.h"
#include "libusbp_config.h"
#include "libusbp.hpp"
#include "protocol.h"
#include "fluxmap.h"
#include "bytes.h"
#include "proto.h"
#include "usbfinder.h"
#include "greaseweazle.h"
#include "fmt/format.h"

static USB* usb = NULL;

USB::~USB()
{}

static std::unique_ptr<CandidateDevice> selectDevice()
{
	auto candidates = findUsbDevices({ FLUXENGINE_ID, GREASEWEAZLE_ID });
	if (candidates.size() == 0)
		Error() << "no devices found (is one plugged in? Do you have the appropriate "
					"permissions?";

	if (config.usb().has_serial())
	{
		auto wantedSerial = config.usb().serial();
		for (auto& c : candidates)
		{
			if (c->serial == wantedSerial)
				return std::move(c);
		}
		Error() << "serial number not found (try without one to list or autodetect devices)";
	}

	if (candidates.size() == 1)
		return std::move(candidates[0]);

	std::cerr << "More than one device detected; use --usb.serial=<serial> to select one:\n";
	for (const auto& c : candidates)
	{
		std::cerr << "    ";
		switch (c->id)
		{
			case FLUXENGINE_ID:
				std::cerr << fmt::format("FluxEngine: {}\n", c->serial);
				break;

			case GREASEWEAZLE_ID:
				std::cerr << fmt::format("GreaseWeazle: {} on {}\n", c->serial, c->serialPort);
				break;
		}
	}
	exit(1);
}

USB* get_usb_impl()
{
	auto candidate = selectDevice();
	switch (candidate->id)
	{
		case FLUXENGINE_ID:
			std::cerr << fmt::format("Using FluxEngine {}\n", candidate->serial);
			return createFluxengineUsb(candidate->device);

		case GREASEWEAZLE_ID:
			std::cerr << fmt::format("Using GreaseWeazle {} on {}\n", candidate->serial, candidate->serialPort);
			return createGreaseWeazleUsb(candidate->serialPort);

		default:
			Error() << "internal";
	}
}

USB& getUsb()
{
	if (!usb)
		usb = get_usb_impl();
	return *usb;
}

