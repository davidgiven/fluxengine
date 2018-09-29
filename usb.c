#include "globals.h"
#include <libusb.h>

#define TIMEOUT 5000

static libusb_device_handle* device;

static uint8_t buffer[FRAME_SIZE];

void usb_init(void)
{
	int i = libusb_init(NULL);
    if (i < 0)
        error("could not start libusb: %s", libusb_strerror(i));

    device = libusb_open_device_with_vid_pid(NULL, FLUXENGINE_VID, FLUXENGINE_PID);
    if (!device)
		error("cannot find FluxEngine (is it plugged in?)");
    
    int cfg = -1;
    libusb_get_configuration(device, &cfg);
    if (cfg != 1)
    {
        i = libusb_set_configuration(device, 1);
        if (i < 0)
            error("FluxEngine would not accept configuration: %s", libusb_strerror(i));
    }

    i = libusb_claim_interface(device, 0);
    if (i < 0)
        error("could not claim interface: %s", libusb_strerror(i));        
}

void usb_cmd_send(void* ptr, int len)
{
    int i = libusb_interrupt_transfer(device, FLUXENGINE_CMD_OUT_EP,
        ptr, len, &len, TIMEOUT);
    if (i < 0)
        error("failed to send command: %s", libusb_strerror(i));
}

void usb_cmd_recv(void* ptr, int len)
{
    int i = libusb_interrupt_transfer(device, FLUXENGINE_CMD_IN_EP,
        ptr, len, &len, TIMEOUT);
    if (i < 0)
        error("failed to receive command reply: %s", libusb_strerror(i));
}

static void bad_reply(void)
{
    struct error_frame* f = (struct error_frame*) buffer;
    if (f->f.type != F_FRAME_ERROR)
        error("bad USB reply %d", f->f.type);
    switch (f->error)
    {
        case F_ERROR_BAD_COMMAND:
            error("device did not understand command");

        default:
            error("unknown error %d", f->error);
    }
}

static void* await_reply(int desired)
{
    usb_cmd_recv(buffer, sizeof(buffer));
    struct any_frame* r = (struct any_frame*) buffer;
    if (r->f.type != desired)
        bad_reply();
    return r;
}

int usb_get_version(void)
{
    struct any_frame f = { .f = {.type = F_FRAME_GET_VERSION_CMD, .size = sizeof(f)} };
    usb_cmd_send(&f, f.f.size);
    struct version_frame* r = await_reply(F_FRAME_GET_VERSION_REPLY);
    return r->version;
}

void usb_seek(int track)
{
    struct seek_frame f = {
        { .type = F_FRAME_SEEK_CMD, .size = sizeof(f) },
        .track = track
    };
    usb_cmd_send(&f, f.f.size);
    await_reply(F_FRAME_SEEK_REPLY);
}
