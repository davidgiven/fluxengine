#include "globals.h"
#include <stdarg.h>

void error(const char* message, ...)
{
    va_list ap;
    va_start(ap, message);
    fprintf(stderr, "fluxengine: ");
    vfprintf(stderr, message, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

int main(int argc, const char** argv)
{
    open_serial_port("COM4");

    for (int i=0; i<10000; i++)
    {
        frame_t frame;
        read_frame(&frame);
    }

    return 0;
}