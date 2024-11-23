#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/config/flags.h"
#include "usb.h"
#include "libusbp_config.h"
#include "libusbp.hpp"
#include "protocol.h"
#include "lib/data/fluxmap.h"
#include "lib/core/bytes.h"
#include "lib/config/proto.h"
#include "usbfinder.h"
#include "lib/core/logger.h"
#include "lib/external/applesauce.h"
#include "lib/external/greaseweazle.h"

static USB* usb = NULL;

USB::~USB() {}

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

            case APPLESAUCE_ID:
                std::cerr << fmt::format(
                    "Applesauce: {} on {}\n", c->serial, c->serialPort);
                break;
        }
    }
    exit(1);
}

USB* get_usb_impl()
{
    /* Special case for certain configurations. */

    if (globalConfig()->usb().has_greaseweazle() &&
        globalConfig()->usb().greaseweazle().has_port())
    {
        const auto& conf = globalConfig()->usb().greaseweazle();
        log("Using Greaseweazle on serial port {}", conf.port());
        return createGreaseweazleUsb(conf.port(), conf);
    }

    if (globalConfig()->usb().has_applesauce() &&
        globalConfig()->usb().applesauce().has_port())
    {
        const auto& conf = globalConfig()->usb().applesauce();
        log("Using Applesauce on serial port {}", conf.port());
        return createApplesauceUsb(conf.port(), conf);
    }

    /* Otherwise, select a device by USB ID. */

    auto candidate = selectDevice();
    switch (candidate->id)
    {
        case FLUXENGINE_ID:
            log("Using FluxEngine {}", candidate->serial);
            return createFluxengineUsb(candidate->device);

        case GREASEWEAZLE_ID:
            log("Using Greaseweazle {} on {}",
                candidate->serial,
                candidate->serialPort);
            return createGreaseweazleUsb(
                candidate->serialPort, globalConfig()->usb().greaseweazle());

        case APPLESAUCE_ID:
            log("Using Applesauce {} on {}",
                candidate->serial,
                candidate->serialPort);
            return createApplesauceUsb(
                candidate->serialPort, globalConfig()->usb().applesauce());

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
