#include "globals.h"
#include "flags.h"
#include "usb.h"
#include "protocol.h"
#include <fmt/format.h>

static FlagGroup flags;

static std::string display_voltages(struct voltages& v)
{
    return fmt::format(
        "      Logic 0 / 1:  {:.2f}V / {:.2f}V\n",
        v.logic0_mv / 1000.0,
        v.logic1_mv / 1000.0);
}

int mainTestVoltages(int argc, const char* argv[])
{
    flags.parseFlags(argc, argv);
    struct voltages_frame f;
    usbMeasureVoltages(&f);

    std::cout << "Output voltages:\n"
              << "  Both drives off\n"
              << display_voltages(f.output_both_off)
              << "  Drive 0 on\n"
              << display_voltages(f.output_drive_0_on)
              << "  Drive 1 on\n"
              << display_voltages(f.output_drive_1_on)
              << "Input voltages:\n"
              << "  Both drives off\n"
              << display_voltages(f.input_both_off)
              << "  Drive 0 on\n"
              << display_voltages(f.input_drive_0_on)
              << "  Drive 1 on\n"
              << display_voltages(f.input_drive_1_on)
              << "\n"
              << "Warning: if you don't have a disk in the drive, the motor may not\n"
              << "start, which may skew your results.\n";

    return 0;
}
