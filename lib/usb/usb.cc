#include "globals.h"
#include "flags.h"
#include "usb.h"
#include "protocol.h"
#include "fluxmap.h"
#include "bytes.h"
#include "proto.h"
#include "usbfinder.h"
#include "fmt/format.h"

static USB* usb = NULL;

USB::~USB()
{}

USB* get_usb_impl()
{
	switch (config.usb().device_case())
	{
		case UsbProto::kFluxengine:
		{
			auto candidates = findUsbDevices(FLUXENGINE_ID);
			for (auto& c : candidates)
			{
				if (c->serial == config.usb().fluxengine())
					return createFluxengineUsb(c->device);
			}
			Error() << "that FluxEngine device could not be found (is it plugged in? Do you have the "
						"appropriate permissions?";
		}

		case UsbProto::kGreaseweazle:
			return createGreaseWeazleUsb(config.usb().greaseweazle());

		default:
		{
			auto candidates = findUsbDevices(FLUXENGINE_ID);
			if (candidates.size() == 0)
				Error() << "no FluxEngine devices found (is one plugged in? Do you have the appropriate "
							"permissions?";
			if (candidates.size() != 1)
			{
				std::cerr << "More than one FluxEngine detected; use --usb.fluxengine=<serial> to\n"
								"select one:\n";
				for (const auto& c : candidates)
					std::cerr << "    " << c->serial << std::endl;
				exit(1);
			}
			return createFluxengineUsb(candidates[0]->device);
		}
	}
}

USB& getUsb()
{
	if (!usb)
		usb = get_usb_impl();
	return *usb;
}

