#include "globals.h"
#include <stdarg.h>
#include <sys/time.h>
#include <unistd.h>
#include <libusb.h>

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

double gettime(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return (double)tv.tv_sec + tv.tv_usec/1000000.0;
}

int countargs(char* const* argv)
{
	int count = 0;
	while (*argv++)
		count++;
	return count;
}

void syntax_error(void)
{
	error("syntax error (try -h for help)");
}

static char* const* parse_global_options(char* const* argv)
{
	for (;;)
	{
		switch (getopt(countargs(argv), argv, "+ht:"))
		{
			case -1:
				return argv + optind;

			case 'h':
				error("sorry, no help yet");

			default:
				syntax_error();
		}
	}
}

int main(int argc, char* const* argv)
{
	argv = parse_global_options(argv);
	usb_init();

	/* Just check that a FluxEngine is plugged in. */

	(void) usb_get_version();

	if (!argv[0])
		syntax_error();

	if (strcmp(argv[0], "rpm") == 0)
		cmd_rpm(argv);
    else if (strcmp(argv[0], "usbbench") == 0)
        cmd_usbbench(argv);
    else if (strcmp(argv[0], "read") == 0)
        cmd_read(argv);
    else if (strcmp(argv[0], "write") == 0)
        cmd_write(argv);
    else if (strcmp(argv[0], "decode") == 0)
        cmd_decode(argv);
    else
        syntax_error();

    return 0;
}