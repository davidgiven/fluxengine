#include "lib/globals.h"
#include "lib/logger.h"
#include <sys/time.h>
#include <stdarg.h>

double getCurrentTime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return double(tv.tv_sec) + tv.tv_usec / 1000000.0;
}

void warning(const std::string msg)
{
    log(msg);
}
