#include "lib/core/globals.h"
#include "usb.h"
#include "protocol.h"
#include "lib/data/fluxmap.h"
#include "lib/core/bytes.h"
#include "libusbp_config.h"
#include "libusbp.hpp"

#define MAX_TRANSFER (32 * 1024)

/* Hacky: the board always operates in little-endian mode. */
static uint16_t read_short_from_usb(uint16_t usb)
{
    uint8_t* p = (uint8_t*)&usb;
    return p[0] | (p[1] << 8);
}

class FluxEngineUsb : public USB
{
private:
    uint8_t _buffer[FRAME_SIZE];

    void usb_cmd_send(void* ptr, size_t len)
    {
        size_t rlen;
        _handle.write_pipe(FLUXENGINE_CMD_OUT_EP, ptr, len, &rlen);
    }

    void usb_cmd_recv(void* ptr, size_t len)
    {
        size_t rlen;
        _handle.read_pipe(FLUXENGINE_CMD_IN_EP, ptr, len, &rlen);
    }

    void usb_data_send(const Bytes& bytes)
    {
        size_t ptr = 0;
        while (ptr < bytes.size())
        {
            size_t rlen = bytes.size() - ptr;
            if (rlen > MAX_TRANSFER)
                rlen = MAX_TRANSFER;
            _handle.write_pipe(
                FLUXENGINE_DATA_OUT_EP, bytes.cbegin() + ptr, rlen, &rlen);
            ptr += rlen;
        }
    }

    void usb_data_recv(Bytes& bytes)
    {
        size_t ptr = 0;
        while (ptr < bytes.size())
        {
            size_t rlen = bytes.size() - ptr;
            if (rlen > MAX_TRANSFER)
                rlen = MAX_TRANSFER;
            _handle.read_pipe(
                FLUXENGINE_DATA_IN_EP, bytes.begin() + ptr, rlen, &rlen);
            ptr += rlen;
            if (rlen < MAX_TRANSFER)
                break;
        }

        bytes.resize(ptr);
    }

public:
    FluxEngineUsb(libusbp::device& device):
        _device(device),
        _interface(_device, 0, false),
        _handle(_interface)
    {
        int version = getVersion();
        if (version != FLUXENGINE_PROTOCOL_VERSION)
            error(
                "your FluxEngine firmware is at version {} but the client is "
                "for version {}; please upgrade",
                version,
                (int)FLUXENGINE_PROTOCOL_VERSION);
    }

private:
    libusbp::device _device;
    libusbp::generic_interface _interface;
    libusbp::generic_handle _handle;

private:
    void bad_reply(void)
    {
        struct error_frame* f = (struct error_frame*)_buffer;
        if (f->f.type != F_FRAME_ERROR)
            error("bad USB reply 0x{:2x}", f->f.type);
        switch (f->error)
        {
            case F_ERROR_BAD_COMMAND:
                error("device did not understand command");

            case F_ERROR_UNDERRUN:
                error("USB underrun (not enough bandwidth)");

            default:
                error("unknown device error {}", f->error);
        }
    }

    template <typename T>
    T* await_reply(int desired)
    {
        for (;;)
        {
            usb_cmd_recv(_buffer, sizeof(_buffer));
            struct any_frame* r = (struct any_frame*)_buffer;
            if (r->f.type == F_FRAME_DEBUG)
            {
                std::cout << "dev: " << ((struct debug_frame*)r)->payload
                          << std::endl;
                continue;
            }
            if (r->f.type != desired)
                bad_reply();
            return (T*)r;
        }
    }

    int getVersion()
    {
        struct any_frame f = {
            .f = {.type = F_FRAME_GET_VERSION_CMD, .size = sizeof(f)}
        };
        usb_cmd_send(&f, f.f.size);
        auto r = await_reply<struct version_frame>(F_FRAME_GET_VERSION_REPLY);
        return r->version;
    }

public:
    void seek(int track) override
    {
        struct seek_frame f = {
            .f = {.type = F_FRAME_SEEK_CMD, .size = sizeof(f)},
            .track = (uint8_t)track
        };
        usb_cmd_send(&f, f.f.size);
        await_reply<struct any_frame>(F_FRAME_SEEK_REPLY);
    }

    void recalibrate() override
    {
        struct any_frame f = {
            .f = {.type = F_FRAME_RECALIBRATE_CMD, .size = sizeof(f)},
        };
        usb_cmd_send(&f, f.f.size);
        await_reply<struct any_frame>(F_FRAME_RECALIBRATE_REPLY);
    }

    nanoseconds_t getRotationalPeriod(int hardSectorCount) override
    {
        struct measurespeed_frame f = {
            .f = {.type = F_FRAME_MEASURE_SPEED_CMD, .size = sizeof(f)},
            .hard_sector_count = (uint8_t)hardSectorCount,
        };
        usb_cmd_send(&f, f.f.size);

        auto r = await_reply<struct speed_frame>(F_FRAME_MEASURE_SPEED_REPLY);
        return r->period_ms * 1000000;
    }

    void testBulkWrite() override
    {
        struct any_frame f = {
            .f = {.type = F_FRAME_BULK_WRITE_TEST_CMD, .size = sizeof(f)}
        };
        usb_cmd_send(&f, f.f.size);

        /* These must match the device. */
        const int XSIZE = 64;
        const int YSIZE = 256;
        const int ZSIZE = 64;

        std::cout << "Reading data: " << std::flush;
        Bytes bulk_buffer(XSIZE * YSIZE * ZSIZE);
        double start_time = getCurrentTime();
        usb_data_recv(bulk_buffer);
        double elapsed_time = getCurrentTime() - start_time;

        std::cout << "transferred " << bulk_buffer.size()
                  << " bytes from device -> PC in "
                  << int(elapsed_time * 1000.0) << " ms ("
                  << int((bulk_buffer.size() / 1024.0) / elapsed_time)
                  << " kB/s)" << std::endl;

        for (int x = 0; x < XSIZE; x++)
            for (int y = 0; y < YSIZE; y++)
                for (int z = 0; z < ZSIZE; z++)
                {
                    int offset = x * XSIZE * YSIZE + y * ZSIZE + z;
                    if (bulk_buffer[offset] != uint8_t(x + y + z))
                        error("data transfer corrupted at 0x{:x} {}.{}.{}",
                            offset,
                            x,
                            y,
                            z);
                }

        await_reply<struct any_frame>(F_FRAME_BULK_WRITE_TEST_REPLY);
    }

    void testBulkRead() override
    {
        struct any_frame f = {
            .f = {.type = F_FRAME_BULK_READ_TEST_CMD, .size = sizeof(f)}
        };
        usb_cmd_send(&f, f.f.size);

        /* These must match the device. */
        const int XSIZE = 64;
        const int YSIZE = 256;
        const int ZSIZE = 64;

        Bytes bulk_buffer(XSIZE * YSIZE * ZSIZE);
        for (int x = 0; x < XSIZE; x++)
            for (int y = 0; y < YSIZE; y++)
                for (int z = 0; z < ZSIZE; z++)
                {
                    int offset = x * XSIZE * YSIZE + y * ZSIZE + z;
                    bulk_buffer[offset] = uint8_t(x + y + z);
                }

        std::cout << "Writing data: " << std::flush;
        double start_time = getCurrentTime();
        usb_data_send(bulk_buffer);
        double elapsed_time = getCurrentTime() - start_time;

        std::cout << "transferred " << bulk_buffer.size()
                  << " bytes from PC -> device in "
                  << int(elapsed_time * 1000.0) << " ms ("
                  << int((bulk_buffer.size() / 1024.0) / elapsed_time)
                  << " kB/s)" << std::endl;

        await_reply<struct any_frame>(F_FRAME_BULK_READ_TEST_REPLY);
    }

    Bytes read(int side,
        bool synced,
        nanoseconds_t readTime,
        nanoseconds_t hardSectorThreshold) override
    {
        struct read_frame f = {
            .f = {.type = F_FRAME_READ_CMD, .size = sizeof(f)},
            .side = (uint8_t)side,
            .synced = (uint8_t)synced,
        };
        f.hardsec_threshold_ms =
            (hardSectorThreshold + 5e5) / 1e6; /* round to nearest ms */
        uint16_t milliseconds = readTime / 1e6;
        ((uint8_t*)&f.milliseconds)[0] = milliseconds;
        ((uint8_t*)&f.milliseconds)[1] = milliseconds >> 8;
        usb_cmd_send(&f, f.f.size);

        auto fluxmap = std::unique_ptr<Fluxmap>(new Fluxmap);

        Bytes buffer(1024 * 1024);
        usb_data_recv(buffer);

        await_reply<struct any_frame>(F_FRAME_READ_REPLY);
        return buffer;
    }

    void write(int side,
        const Bytes& bytes,
        nanoseconds_t hardSectorThreshold) override
    {
        unsigned safelen = bytes.size() & ~(FRAME_SIZE - 1);
        Bytes safeBytes = bytes.slice(0, safelen);

        struct write_frame f = {
            .f = {.type = F_FRAME_WRITE_CMD, .size = sizeof(f)},
            .side = (uint8_t)side,
        };
        f.hardsec_threshold_ms =
            (hardSectorThreshold + 5e5) / 1e6; /* round to nearest ms */
        ((uint8_t*)&f.bytes_to_write)[0] = safelen;
        ((uint8_t*)&f.bytes_to_write)[1] = safelen >> 8;
        ((uint8_t*)&f.bytes_to_write)[2] = safelen >> 16;
        ((uint8_t*)&f.bytes_to_write)[3] = safelen >> 24;

        usb_cmd_send(&f, f.f.size);
        usb_data_send(safeBytes);

        await_reply<struct any_frame>(F_FRAME_WRITE_REPLY);
    }

    void erase(int side, nanoseconds_t hardSectorThreshold) override
    {
        struct erase_frame f = {
            .f = {.type = F_FRAME_ERASE_CMD, .size = sizeof(f)},
            .side = (uint8_t)side,
        };
        f.hardsec_threshold_ms =
            (hardSectorThreshold + 5e5) / 1e6; /* round to nearest ms */
        usb_cmd_send(&f, f.f.size);

        await_reply<struct any_frame>(F_FRAME_ERASE_REPLY);
    }

    void setDrive(int drive, bool high_density, int index_mode) override
    {
        struct set_drive_frame f = {
            .f = {.type = F_FRAME_SET_DRIVE_CMD, .size = sizeof(f)},
            .drive = (uint8_t)drive,
            .high_density = high_density,
            .index_mode = (uint8_t)index_mode
        };
        usb_cmd_send(&f, f.f.size);
        await_reply<struct any_frame>(F_FRAME_SET_DRIVE_REPLY);
    }

    void measureVoltages(struct voltages_frame* voltages) override
    {
        struct any_frame f = {
            {.type = F_FRAME_MEASURE_VOLTAGES_CMD, .size = sizeof(f)},
        };
        usb_cmd_send(&f, f.f.size);

        auto convert_voltages_from_usb =
            [&](const struct voltages& vin, struct voltages& vout)
        {
            vout.logic0_mv = read_short_from_usb(vin.logic0_mv);
            vout.logic1_mv = read_short_from_usb(vin.logic1_mv);
        };

        struct voltages_frame* r =
            await_reply<struct voltages_frame>(F_FRAME_MEASURE_VOLTAGES_REPLY);
        convert_voltages_from_usb(r->input_both_off, voltages->input_both_off);
        convert_voltages_from_usb(
            r->input_drive_0_selected, voltages->input_drive_0_selected);
        convert_voltages_from_usb(
            r->input_drive_1_selected, voltages->input_drive_1_selected);
        convert_voltages_from_usb(
            r->input_drive_0_running, voltages->input_drive_0_running);
        convert_voltages_from_usb(
            r->input_drive_1_running, voltages->input_drive_1_running);
        convert_voltages_from_usb(
            r->output_both_off, voltages->output_both_off);
        convert_voltages_from_usb(
            r->output_drive_0_selected, voltages->output_drive_0_selected);
        convert_voltages_from_usb(
            r->output_drive_1_selected, voltages->output_drive_1_selected);
        convert_voltages_from_usb(
            r->output_drive_0_running, voltages->output_drive_0_running);
        convert_voltages_from_usb(
            r->output_drive_1_running, voltages->output_drive_1_running);
    }
};

USB* createFluxengineUsb(libusbp::device& device)
{
    return new FluxEngineUsb(device);
}
