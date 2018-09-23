#include "globals.h"

#if defined __unix__

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

static int handle;

void open_serial_port(const char *name)
{
	handle = open(name, O_RDWR);
	if (handle == -1)
        error("failed to open serial port %s: %s", name, strerror(errno));
}

void read_frame(frame_t* frame)
{
	int bytes_read = read(handle, frame, sizeof(frame_t));
	if (bytes_read == -1)
        error("couldn't read from serial port: %s", strerror(errno));
    if ((bytes_read != sizeof(frame_t)) || (frame->id != FLUXENGINE_ID))
        error("sequencing error");
}


#endif
