#include "globals.h"

static void syntax_error(void)
{
    fprintf(stderr, "syntax: fluxclient usbbench\n");
    exit(1);
}

void cmd_usbbench(char* const* argv)
{
    if (countargs(argv) != 1)
        syntax_error();

    usb_bulk_test();
}

