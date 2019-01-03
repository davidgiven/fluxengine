#include "globals.h"
#include "usb.h"
#include "protocol.h"
#include "fluxmap.h"
#include <libusb.h>

#define TIMEOUT 5000

static libusb_device_handle* device;

static uint8_t buffer[FRAME_SIZE];

static std::string usberror(int i)
{
    return libusb_strerror((libusb_error) i);
}

static void usb_init()
{
    if (device)
        return;

    int i = libusb_init(NULL);
    if (i < 0)
        Error() << "could not start libusb: " << usberror(i);

    device = libusb_open_device_with_vid_pid(NULL, FLUXENGINE_VID, FLUXENGINE_PID);
    if (!device)
		Error() << "cannot find the FluxEngine (is it plugged in?)";
    
    int cfg = -1;
    libusb_get_configuration(device, &cfg);
    if (cfg != 1)
    {
        i = libusb_set_configuration(device, 1);
        if (i < 0)
            Error() << "the FluxEngine would not accept configuration: " << usberror(i);
    }

    i = libusb_claim_interface(device, 0);
    if (i < 0)
        Error() << "could not claim interface: " << usberror(i);

    int version = usbGetVersion();
    if (version > FLUXENGINE_VERSION)
        Error() << "this version of the client is too old for this FluxEngine";
}

static int usb_cmd_send(void* ptr, int len)
{
    int i = libusb_interrupt_transfer(device, FLUXENGINE_CMD_OUT_EP,
        (uint8_t*) ptr, len, &len, TIMEOUT);
    if (i < 0)
        Error() << "failed to send command: " << usberror(i);
    return len;
}

void usb_cmd_recv(void* ptr, int len)
{
    int i = libusb_interrupt_transfer(device, FLUXENGINE_CMD_IN_EP,
       (uint8_t*)  ptr, len, &len, TIMEOUT);
    if (i < 0)
        Error() << "failed to receive command reply: " << usberror(i);
}

static void bad_reply(void)
{
    struct error_frame* f = (struct error_frame*) buffer;
    if (f->f.type != F_FRAME_ERROR)
        Error() << "bad USB reply " << f->f.type;
    switch (f->error)
    {
        case F_ERROR_BAD_COMMAND:
            Error() << "device did not understand command";

        case F_ERROR_UNDERRUN:
            Error() << "USB underrun (not enough bandwidth)";
            
        default:
            Error() << "unknown device error " << f->error;
    }
}

template <typename T>
static T* await_reply(int desired)
{
    usb_cmd_recv(buffer, sizeof(buffer));
    struct any_frame* r = (struct any_frame*) buffer;
    if (r->f.type != desired)
        bad_reply();
    return (T*) r;
}

int usbGetVersion(void)
{
    usb_init();

    struct any_frame f = { .f = {.type = F_FRAME_GET_VERSION_CMD, .size = sizeof(f)} };
    usb_cmd_send(&f, f.f.size);
    auto r = await_reply<struct version_frame>(F_FRAME_GET_VERSION_REPLY);
    return r->version;
}

void usbSeek(int track)
{
    usb_init();

    struct seek_frame f = {
        { .type = F_FRAME_SEEK_CMD, .size = sizeof(f) },
        .track = (uint8_t) track
    };
    usb_cmd_send(&f, f.f.size);
    await_reply<struct any_frame>(F_FRAME_SEEK_REPLY);
}

nanoseconds_t usbGetRotationalPeriod(void)
{
    usb_init();

    struct any_frame f = { .f = {.type = F_FRAME_MEASURE_SPEED_CMD, .size = sizeof(f)} };
    usb_cmd_send(&f, f.f.size);

    auto r = await_reply<struct speed_frame>(F_FRAME_MEASURE_SPEED_REPLY);
    return r->period_ms * 1000;
}

static int large_bulk_transfer(int ep, std::vector<uint8_t>& buffer)
{
    int len;
    int i = libusb_bulk_transfer(device, ep, &buffer[0], buffer.size(), &len, TIMEOUT);
    if (i < 0)
        Error() << "data transfer failed: " << usberror(i);
    return len;
}

void usbTestBulkTransport()
{
    usb_init();

    struct any_frame f = { .f = {.type = F_FRAME_BULK_TEST_CMD, .size = sizeof(f)} };
    usb_cmd_send(&f, f.f.size);

    /* These must match the device. */
    const int XSIZE = 64;
    const int YSIZE = 256;
    const int ZSIZE = 64;

    std::vector<uint8_t> bulk_buffer(XSIZE*YSIZE*ZSIZE);
    double start_time = getCurrentTime();
    large_bulk_transfer(FLUXENGINE_DATA_IN_EP, bulk_buffer);
    double elapsed_time = getCurrentTime() - start_time;

    std::cout << "Transferred "
              << bulk_buffer.size()
              << " bytes in "
              << int(elapsed_time * 1000.0)
              << " ("
              << int((bulk_buffer.size() / 1024.0) / elapsed_time)
              << " kB/s)"
              << std::endl;

    for (int x=0; x<XSIZE; x++)
        for (int y=0; y<YSIZE; y++)
            for (int z=0; z<ZSIZE; z++)
            {
                int offset = x*XSIZE*YSIZE + y*ZSIZE + z;
                if (bulk_buffer.at(offset) != uint8_t(x+y+z))
                    Error() << "data transfer corrupted at 0x"
                            << std::hex << offset << std::dec
                            << " "
                            << x << '.' << y << '.' << z << '.';
            }

    await_reply<struct any_frame>(F_FRAME_BULK_TEST_REPLY);
}

std::unique_ptr<Fluxmap> usbRead(int side, int revolutions)
{
    struct read_frame f = {
        .f = { .type = F_FRAME_READ_CMD, .size = sizeof(f) },
        .side = (uint8_t) side,
        .revolutions = (uint8_t) revolutions
    };
    usb_cmd_send(&f, f.f.size);

    auto fluxmap = std::unique_ptr<Fluxmap>(new Fluxmap);

    std::vector<uint8_t> buffer(1024*1024);
    int len = large_bulk_transfer(FLUXENGINE_DATA_IN_EP, buffer);
    buffer.resize(len);

    fluxmap->appendIntervals(buffer);

    await_reply<struct any_frame>(F_FRAME_READ_REPLY);
    return fluxmap;
}

void usbWrite(int side, const Fluxmap& fluxmap)
{
    int safelen = fluxmap.bytes() & ~(FRAME_SIZE-1);

    /* Convert from intervals to absolute timestamps. */

	std::vector<uint8_t> buffer(safelen);
    uint8_t clock = 0;
    for (int i=0; i<safelen; i++)
    {
        clock += fluxmap[i];
        buffer[i] = clock;
    }

    struct write_frame f = {
        .f = { .type = F_FRAME_WRITE_CMD, .size = sizeof(f) },
        .side = (uint8_t) side,
        .bytes_to_write = htole32(safelen),
    };
    usb_cmd_send(&f, f.f.size);

    large_bulk_transfer(FLUXENGINE_DATA_OUT_EP, buffer);
    
    await_reply<struct any_frame>(F_FRAME_WRITE_REPLY);
}

