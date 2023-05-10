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
#include "logger.h"
#include "greaseweazle.h"

static USB* usb = NULL;

USB::~USB() {}

static std::shared_ptr<CandidateDevice> selectDevice()
{
    auto candidates = findUsbDevices();
    if (candidates.size() == 0)
        error(
            "no devices found (is one plugged in? Do you have the "
            "appropriate permissions?");

    if (globalConfig().usb().has_serial())
    {
        auto wantedSerial = globalConfig().usb().serial();
        for (auto& c : candidates)
        {
            if (c->serial == wantedSerial)
                return c;
        }
        error(
            "serial number not found (try without one to list or "
            "autodetect devices)");
    }

    if (candidates.size() == 1)
        return candidates[0];

    std::cerr << "More than one device detected; use --usb.serial=<serial> to "
                 "select one:\n";
    for (const auto& c : candidates)
    {
        std::cerr << "    ";
        switch (c->id)
        {
            case FLUXENGINE_ID:
                std::cerr << fmt::format("FluxEngine: {}\n", c->serial);
                break;

            case GREASEWEAZLE_ID:
                std::cerr << fmt::format(
                    "GreaseWeazle: {} on {}\n", c->serial, c->serialPort);
                break;
        }
    }
    exit(1);
}

USB* get_usb_impl()
{
    /* Special case for certain configurations. */

    if (globalConfig().usb().has_greaseweazle() &&
        globalConfig().usb().greaseweazle().has_port())
    {
        const auto& conf = globalConfig().usb().greaseweazle();
        log("Using GreaseWeazle on serial port {}", conf.port());
        return createGreaseWeazleUsb(conf.port(), conf);
    }

    /* Otherwise, select a device by USB ID. */

    auto candidate = selectDevice();
    switch (candidate->id)
    {
        case FLUXENGINE_ID:
            log("Using FluxEngine {}", candidate->serial);
            return createFluxengineUsb(candidate->device);

        case GREASEWEAZLE_ID:
            log("Using GreaseWeazle {} on {}",
                candidate->serial,
                candidate->serialPort);
            return createGreaseWeazleUsb(
                candidate->serialPort, globalConfig().usb().greaseweazle());

        default:
            error("internal");
    }
}

USB& getUsb()
{
    if (!usb)
        usb = get_usb_impl();
    return *usb;
}
