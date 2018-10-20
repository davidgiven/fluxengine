#include "globals.h"
#include "fluxmap.h"
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

        case F_ERROR_UNDERRUN:
            error("USB underrun (not enough bandwidth)");
            
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

int usb_measure_speed(void)
{
    struct any_frame f = { .f = {.type = F_FRAME_MEASURE_SPEED_CMD, .size = sizeof(f)} };
    usb_cmd_send(&f, f.f.size);
    struct speed_frame* r = await_reply(F_FRAME_MEASURE_SPEED_REPLY);
    return r->period_ms;
}

static int large_bulk_transfer(int ep, void* buffer, int total_len)
{
    int len;
    int i = libusb_bulk_transfer(device, ep, buffer, total_len, &len, TIMEOUT);
    if (i < 0)
        error("data transfer failed: %s", libusb_strerror(i));
    return len;
}

void usb_bulk_test(void)
{
    struct any_frame f = { .f = {.type = F_FRAME_BULK_TEST_CMD, .size = sizeof(f)} };
    usb_cmd_send(&f, f.f.size);

    uint8_t bulk_buffer[64*256*64];
    int total_len = sizeof(bulk_buffer);
    double start_time = gettime();
    large_bulk_transfer(FLUXENGINE_DATA_IN_EP, bulk_buffer, total_len);
    double elapsed_time = gettime() - start_time;

    printf("Transferred %d bytes in %d ms (%d kB/s)\n",
        total_len, (int)(elapsed_time * 1000.0),
        (int)((total_len / 1024.0) / elapsed_time));
    for (int x=0; x<64; x++)
        for (int y=0; y<256; y++)
            for (int z=0; z<64; z++)
            {
                int offset = x*16384 + y*64 + z;
                if (bulk_buffer[offset] != (uint8_t)(x+y+z))
                    error("data transfer corrupted at 0x%x (%d.%d.%d)", offset, x, y, z);
            }

    await_reply(F_FRAME_BULK_TEST_REPLY);
}

struct fluxmap* usb_read(int side, int revolutions)
{
    struct read_frame f = {
        .f = { .type = F_FRAME_READ_CMD, .size = sizeof(f) },
        .side = side,
        .revolutions = revolutions
    };

    struct fluxmap* fluxmap = create_fluxmap();
    usb_cmd_send(&f, f.f.size);

    uint8_t buffer[1024*1024];
    int len = large_bulk_transfer(FLUXENGINE_DATA_IN_EP, buffer, sizeof(buffer));

    fluxmap_append_intervals(fluxmap, buffer, len);

    await_reply(F_FRAME_READ_REPLY);
    return fluxmap;
}

/* Returns number of bytes actually written */
void usb_write(int side, struct fluxmap* fluxmap)
{
    int safelen = fluxmap->bytes & ~(FRAME_SIZE-1);

    /* Convert from intervals to absolute timestamps. */

    uint8_t buffer[1024*1024];
    uint8_t clock = 0;
    for (int i=0; i<safelen; i++)
    {
        clock += fluxmap->intervals[i];
        buffer[i] = clock;
    }

    struct write_frame f = {
        .f = { .type = F_FRAME_WRITE_CMD, .size = sizeof(f) },
        .side = side,
        .bytes_to_write = htole32(safelen),
    };
    usb_cmd_send(&f, f.f.size);

    large_bulk_transfer(FLUXENGINE_DATA_OUT_EP, buffer, safelen);
    
    await_reply(F_FRAME_WRITE_REPLY);
}
