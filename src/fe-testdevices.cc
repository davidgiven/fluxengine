#include "lib/core/globals.h"
#include "lib/config/flags.h"
#include "lib/usb/usbfinder.h"
#include "fmt/format.h"

static FlagGroup flags;

int mainTestDevices(int argc, const char* argv[])
{
    flags.parseFlagsWithConfigFiles(argc, argv, {});

    auto candidates = findUsbDevices();
    switch (candidates.size())
    {
        case 0:
            fmt::print("Detected no devices.\n");
            break;

        case 1:
            fmt::print("Detected one device:\n");
            break;

        default:
            fmt::print("Detected {} devices:\n", candidates.size());
    }

    if (!candidates.empty())
    {
        fmt::print(
            "{:15} {:30} {}\n", "Type", "Serial number", "Port (if any)");
        for (auto& candidate : candidates)
        {
            fmt::print("{:15} {:30} {}\n",
                getDeviceName(candidate->type),
                candidate->serial,
                candidate->serialPort);
        }
    }

    return 0;
}
