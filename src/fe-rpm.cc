#include "globals.h"
#include "flags.h"
#include "usb.h"
#include "dataspec.h"

static DataSpecFlag source(
    { "--source", "-s" },
    "source for data",
    ":d=0");

int main(int argc, const char* argv[])
{
    Flag::parseFlags(argc, argv);

    usbSetDrive(source.value.drive);
    nanoseconds_t period = usbGetRotationalPeriod();
    std::cout << "Rotational period is " << period/1000 << " ms (" << 60e6/period << " rpm)" << std::endl;

    return 0;
}
