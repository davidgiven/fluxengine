#include "lib/globals.h"
#include "lib/flags.h"
#include "usb.h"
#include "libusbp_config.h"
#include "libusbp.hpp"
#include "protocol.h"
#include "lib/fluxmap.h"
#include "lib/bytes.h"
#include "lib/proto.h"
#include "usbfinder.h"
#include "lib/logger.h"
#include "greaseweazle.h"

static USB* usb;

USB::~USB()
{
    usb = nullptr;
}

static std::shared_ptr<CandidateDevice> selectDevice()
{
    auto candidates = findUsbDevices();
    if (candidates.size() == 0)
        error(
            "no devices found (is one plugged in? Do you have the "
            "appropriate permissions?");

    if (globalConfig()->usb().has_serial())
    {
        auto wantedSerial = globalConfig()->usb().serial();
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
                    "Greaseweazle: {} on {}\n", c->serial, c->serialPort);
                break;
        }
    }
    exit(1);
}

std::unique_ptr<USB> USB::create()
{
    std::unique_ptr<USB> r;
    if (usb)
        error("more than one USB object created");

    /* Special case for certain configurations. */

    if (globalConfig()->usb().has_greaseweazle() &&
        globalConfig()->usb().greaseweazle().has_port())
    {
        const auto& conf = globalConfig()->usb().greaseweazle();
        log("Using Greaseweazle on serial port {}", conf.port());
        r.reset(createGreaseweazleUsb(conf.port(), conf));
    }
    else
    {
        /* Otherwise, select a device by USB ID. */

        auto candidate = selectDevice();
        switch (candidate->id)
        {
            case FLUXENGINE_ID:
                log("Using FluxEngine {}", candidate->serial);
                r.reset(createFluxengineUsb(candidate->device));
                break;

            case GREASEWEAZLE_ID:
                log("Using Greaseweazle {} on {}",
                    candidate->serial,
                    candidate->serialPort);
                r.reset(createGreaseweazleUsb(candidate->serialPort,
                    globalConfig()->usb().greaseweazle()));
                break;

            default:
                error("internal");
        }
    }

    usb = r.get();
    return r;
}

USB& getUsb()
{
    if (!usb)
        error("USB instance not created");
    return *usb;
}
