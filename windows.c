#include "globals.h"

#if defined WINDOWS

#include <windows.h>

static HANDLE handle;
static char error_buffer[128];

static const char* get_windows_error(void)
{
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
               NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
               error_buffer, sizeof(error_buffer), NULL);
    return error_buffer;
}

void open_serial_port(const char *name)
{
    handle = CreateFile(name, GENERIC_READ | GENERIC_WRITE,
                        0,
                        0,
                        OPEN_EXISTING,
                        0,
                        0);
    if (handle == INVALID_HANDLE_VALUE)
        error("failed to open serial port %s: %s", name, get_windows_error());
}

void read_frame(frame_t* frame)
{
    DWORD bytes_read = 0;
    if (!ReadFile(handle, frame, sizeof(frame_t), &bytes_read, NULL))
        error("couldn't read from serial port: %s", get_windows_error());
    if ((bytes_read != sizeof(frame_t)) || (frame->id != FLUXENGINE_ID))
        error("sequencing error");
}

#endif
