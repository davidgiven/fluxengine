#include "globals.h"
#include "flags.h"
#include "usb.h"
#include "bytes.h"
#include "usbfinder.h"
#include "greaseweazle.h"
#include "protocol.h"
#include "libusbp.hpp"

static const std::set<uint32_t> VALID_DEVICES = {
    GREASEWEAZLE_ID, FLUXENGINE_ID};

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

std::vector<std::shared_ptr<CandidateDevice>> findUsbDevices()
{
    std::vector<std::shared_ptr<CandidateDevice>> candidates;
    for (const auto& it : libusbp::list_connected_devices())
    {
        auto candidate = std::make_unique<CandidateDevice>();
        candidate->device = it;

        uint32_t id = (it.get_vendor_id() << 16) | it.get_product_id();
        if (VALID_DEVICES.find(id) != VALID_DEVICES.end())
        {
            candidate->id = id;
            candidate->serial = get_serial_number(it);

            if (id == GREASEWEAZLE_ID)
            {
                libusbp::serial_port port(candidate->device);
                candidate->serialPort = port.get_name();
                candidate->type = DEVICE_GREASEWEAZLE;
            }
            else if (id == FLUXENGINE_ID)
                candidate->type = DEVICE_FLUXENGINE;

            candidates.push_back(std::move(candidate));
        }
    }

    return candidates;
}

std::string getDeviceName(DeviceType type)
{
    switch (type)
    {
        case DEVICE_GREASEWEAZLE:
            return "Greaseweazle";

        case DEVICE_FLUXENGINE:
            return "FluxEngine";

        default:
            return "unknown";
    }
}
