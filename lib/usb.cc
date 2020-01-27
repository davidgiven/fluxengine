#include "globals.h"
#include "usb.h"
#include "protocol.h"
#include "fluxmap.h"
#include "bytes.h"
#include "common/crunch.h"
#include <libusb.h>
#include "fmt/format.h"

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
    if (version != FLUXENGINE_VERSION)
        Error() << "your FluxEngine firmware is at version " << version
                << " but the client is for version " << FLUXENGINE_VERSION
                << "; please upgrade";
}

static int usb_cmd_send(void* ptr, int len)
{
    //std::cerr << "send:\n";
    //hexdump(std::cerr, Bytes((const uint8_t*)ptr, len));
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
    //std::cerr << "recv:\n";
    //hexdump(std::cerr, Bytes((const uint8_t*)ptr, len));
}

static void bad_reply(void)
{
    struct error_frame* f = (struct error_frame*) buffer;
    if (f->f.type != F_FRAME_ERROR)
        Error() << fmt::format("bad USB reply 0x{:2x}", f->f.type);
    switch (f->error)
    {
        case F_ERROR_BAD_COMMAND:
            Error() << "device did not understand command";

        case F_ERROR_UNDERRUN:
            Error() << "USB underrun (not enough bandwidth)";
            
        default:
            Error() << fmt::format("unknown device error {}", f->error);
    }
}

template <typename T>
static T* await_reply(int desired)
{
    for (;;)
    {
        usb_cmd_recv(buffer, sizeof(buffer));
        struct any_frame* r = (struct any_frame*) buffer;
        if (r->f.type == F_FRAME_DEBUG)
        {
            std::cout << "dev: " << ((struct debug_frame*)r)->payload << std::endl;
            continue;
        }
        if (r->f.type != desired)
            bad_reply();
        return (T*) r;
    }
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

void usbRecalibrate()
{
    usb_init();

    struct any_frame f = {
        { .type = F_FRAME_RECALIBRATE_CMD, .size = sizeof(f) },
    };
    usb_cmd_send(&f, f.f.size);
    await_reply<struct any_frame>(F_FRAME_RECALIBRATE_REPLY);
}

nanoseconds_t usbGetRotationalPeriod(void)
{
    usb_init();

    struct any_frame f = { .f = {.type = F_FRAME_MEASURE_SPEED_CMD, .size = sizeof(f)} };
    usb_cmd_send(&f, f.f.size);

    auto r = await_reply<struct speed_frame>(F_FRAME_MEASURE_SPEED_REPLY);
    return r->period_ms * 1000000;
}

static int large_bulk_transfer(int ep, Bytes& bytes)
{
    int len;
    int i = libusb_bulk_transfer(device, ep, bytes.begin(), bytes.size(), &len, TIMEOUT);
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

    Bytes bulk_buffer(XSIZE*YSIZE*ZSIZE);
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
                if (bulk_buffer[offset] != uint8_t(x+y+z))
                    Error() << "data transfer corrupted at 0x"
                            << std::hex << offset << std::dec
                            << " "
                            << x << '.' << y << '.' << z << '.';
            }

    await_reply<struct any_frame>(F_FRAME_BULK_TEST_REPLY);
}

Bytes usbRead(int side, bool synced, nanoseconds_t readTime)
{
    struct read_frame f = {
        .f = { .type = F_FRAME_READ_CMD, .size = sizeof(f) },
        .side = (uint8_t) side,
        .synced = (uint8_t) synced
    };
    uint16_t milliseconds = readTime / 1e6;
    ((uint8_t*)&f.milliseconds)[0] = milliseconds;
    ((uint8_t*)&f.milliseconds)[1] = milliseconds >> 8;
    usb_cmd_send(&f, f.f.size);

    auto fluxmap = std::unique_ptr<Fluxmap>(new Fluxmap);

    Bytes buffer(1024*1024);
    int len = large_bulk_transfer(FLUXENGINE_DATA_IN_EP, buffer);
    buffer.resize(len);

    await_reply<struct any_frame>(F_FRAME_READ_REPLY);
    return buffer;
}

void usbWrite(int side, const Bytes& bytes)
{
    unsigned safelen = bytes.size() & ~(FRAME_SIZE-1);
    Bytes safeBytes = bytes.slice(0, safelen);

    struct write_frame f = {
        .f = { .type = F_FRAME_WRITE_CMD, .size = sizeof(f) },
        .side = (uint8_t) side,
    };
    ((uint8_t*)&f.bytes_to_write)[0] = safelen;
    ((uint8_t*)&f.bytes_to_write)[1] = safelen >> 8;
    ((uint8_t*)&f.bytes_to_write)[2] = safelen >> 16;
    ((uint8_t*)&f.bytes_to_write)[3] = safelen >> 24;

    usb_cmd_send(&f, f.f.size);

    large_bulk_transfer(FLUXENGINE_DATA_OUT_EP, safeBytes);
    
    await_reply<struct any_frame>(F_FRAME_WRITE_REPLY);
}

void usbErase(int side)
{
    struct erase_frame f = {
        .f = { .type = F_FRAME_ERASE_CMD, .size = sizeof(f) },
        .side = (uint8_t) side,
    };
    usb_cmd_send(&f, f.f.size);

    await_reply<struct any_frame>(F_FRAME_ERASE_REPLY);
}

void usbSetDrive(int drive, bool high_density, int index_mode)
{
    usb_init();

    struct set_drive_frame f = {
        { .type = F_FRAME_SET_DRIVE_CMD, .size = sizeof(f) },
        .drive = (uint8_t) drive,
        .high_density = high_density,
        .index_mode = (uint8_t) index_mode
    };
    usb_cmd_send(&f, f.f.size);
    await_reply<struct any_frame>(F_FRAME_SET_DRIVE_REPLY);
}

/* Hacky: the board always operates in little-endian mode. */
static uint16_t read_short_from_usb(uint16_t usb)
{
    uint8_t* p = (uint8_t*)&usb;
    return p[0] | (p[1] << 8);
}

static void convert_voltages_from_usb(const struct voltages& vin, struct voltages& vout)
{
    vout.logic0_mv = read_short_from_usb(vin.logic0_mv);
    vout.logic1_mv = read_short_from_usb(vin.logic1_mv);
}

void usbMeasureVoltages(struct voltages_frame* voltages)
{
    usb_init();

    struct any_frame f = {
        { .type = F_FRAME_MEASURE_VOLTAGES_CMD, .size = sizeof(f) },
    };
    usb_cmd_send(&f, f.f.size);

    struct voltages_frame* r = await_reply<struct voltages_frame>(F_FRAME_MEASURE_VOLTAGES_REPLY);
    convert_voltages_from_usb(r->input_both_off, voltages->input_both_off);
    convert_voltages_from_usb(r->input_drive_0_selected, voltages->input_drive_0_selected);
    convert_voltages_from_usb(r->input_drive_1_selected, voltages->input_drive_1_selected);
    convert_voltages_from_usb(r->input_drive_0_running, voltages->input_drive_0_running);
    convert_voltages_from_usb(r->input_drive_1_running, voltages->input_drive_1_running);
    convert_voltages_from_usb(r->output_both_off, voltages->output_both_off);
    convert_voltages_from_usb(r->output_drive_0_selected, voltages->output_drive_0_selected);
    convert_voltages_from_usb(r->output_drive_1_selected, voltages->output_drive_1_selected);
    convert_voltages_from_usb(r->output_drive_0_running, voltages->output_drive_0_running);
    convert_voltages_from_usb(r->output_drive_1_running, voltages->output_drive_1_running);
}
