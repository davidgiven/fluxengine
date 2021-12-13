#include "globals.h"
#include "flags.h"
#include "usb.h"
#include "bytes.h"
#include "fmt/format.h"
#include "usbfinder.h"
#include "greaseweazle.h"
#include "libusbp.hpp"

static const std::string get_serial_number(const libusbp::device& device)
{
	try
	{
		return device.get_serial_number();
	}
	catch (const libusbp::error& e)
	{
		if (e.has_code(LIBUSBP_ERROR_NO_SERIAL_NUMBER))
			return "n/a";
		throw;
	}
}

std::vector<std::unique_ptr<CandidateDevice>> findUsbDevices(const std::set<uint32_t>& ids)
{
	std::vector<std::unique_ptr<CandidateDevice>> candidates;
	for (const auto& it : libusbp::list_connected_devices())
	{
		auto candidate = std::make_unique<CandidateDevice>();
		candidate->device = it;

		uint32_t id = (it.get_vendor_id() << 16) | it.get_product_id();
		if (ids.contains(id))
		{
			candidate->id = id;
			candidate->serial = get_serial_number(it);

			if (id == GREASEWEAZLE_ID)
			{
				libusbp::serial_port port(candidate->device);
				candidate->serialPort = port.get_name();
			}

			candidates.push_back(std::move(candidate));
		}
	}

	return candidates;
}
