#include "globals.h"
#include <stdarg.h>
#include <sys/time.h>
#include <unistd.h>

static const char* serialport = NULL;

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

static void parse_options(int argc, char* const* argv)
{
	for (;;)
	{
		switch (getopt(argc, argv, "ht:"))
		{
			case -1:
				return;

			case 'h':
				error("sorry, no help yet");

			case 't':
				serialport = optarg;
				break;

			default:
				error("syntax error (try -h for help)");
		}
	}
}

int main(int argc, char* const* argv)
{
	parse_options(argc, argv);
	if (!serialport)
		error("you must specify a serial port name");
    open_serial_port(serialport);

	const int iterations = 10000;
	double starttime = gettime();
    for (int i=0; i<iterations; i++)
    {
        frame_t frame;
        read_frame(&frame);
    }
    double endtime = gettime();
    int transferred = iterations*sizeof(frame_t);
    printf("amount transferred: %d bytes\n", transferred);
    double elapsedtime = endtime - starttime;
    printf("elapsed time: %f seconds\n", elapsedtime);
    printf("performance: %f kB/s\n", (transferred/1024) / elapsedtime);

    return 0;
}