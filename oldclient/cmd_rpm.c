#include "globals.h"

static void syntax_error(void)
{
    fprintf(stderr, "syntax: fluxclient rpm\n");
    exit(1);
}

void cmd_rpm(char* const* argv)
{
    if (countargs(argv) != 1)
        syntax_error();

    int period_ms = usb_measure_speed();
    printf("Rotational period is %d ms (%f rpm)\n", period_ms, 60000.0/period_ms);
}
