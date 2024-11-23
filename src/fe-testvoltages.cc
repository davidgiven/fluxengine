#include "lib/core/globals.h"
#include "lib/config/flags.h"
#include "lib/usb/usb.h"
#include "protocol.h"

static FlagGroup flags;

static std::string display_voltages(struct voltages& v)
{
    return fmt::format("      Logic 1 / 0:  {:.2f}V / {:.2f}V\n",
        v.logic0_mv / 1000.0,
        v.logic1_mv / 1000.0);
}

int mainTestVoltages(int argc, const char* argv[])
{
    flags.parseFlagsWithConfigFiles(argc, argv, {});
    struct voltages_frame f;
    usbMeasureVoltages(&f);

    std::cout
        << "Output voltages:\n"
        << "  Both drives deselected\n"
        << display_voltages(f.output_both_off) << "  Drive 0 selected\n"
        << display_voltages(f.output_drive_0_selected) << "  Drive 1 selected\n"
        << display_voltages(f.output_drive_1_selected) << "  Drive 0 running\n"
        << display_voltages(f.output_drive_0_running) << "  Drive 1 running\n"
        << display_voltages(f.output_drive_1_running) << "Input voltages:\n"
        << "  Both drives deselected\n"
        << display_voltages(f.input_both_off) << "  Drive 0 selected\n"
        << display_voltages(f.input_drive_0_selected) << "  Drive 1 selected\n"
        << display_voltages(f.input_drive_1_selected) << "  Drive 0 running\n"
        << display_voltages(f.input_drive_0_running) << "  Drive 1 running\n"
        << display_voltages(f.input_drive_1_running);

    return 0;
}
