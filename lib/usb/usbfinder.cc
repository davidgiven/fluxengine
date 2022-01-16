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

    if (candidates.size() == 0) {
      for (const auto& it : libusbp::list_connected_devices())
        {
          auto candidate = std::make_unique<CandidateDevice>();
          candidate->device = it;
          uint32_t id = (it.get_vendor_id() << 16) | it.get_product_id();
          candidate->id = id;
          candidate->serial = get_serial_number(it);
          printf("USB ID %04x %04x: ", it.get_vendor_id(), it.get_product_id());
          try
            {
              libusbp::serial_port port(candidate->device, 0, true);
              candidate->serialPort = port.get_name();
              printf("generic serialPort found\n");
              candidates.push_back(std::move(candidate));
            }
          catch(const libusbp::error & error)
            {
              // not a serial port!
              printf("not a port!\n");
              continue;
            }

		}
	}
	return candidates;
}
