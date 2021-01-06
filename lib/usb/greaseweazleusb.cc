#include "globals.h"
#include "usb.h"
#include "protocol.h"
#include "fluxmap.h"
#include "bytes.h"
#include <libusb.h>
#include "fmt/format.h"
#include "greaseweazle.h"

#define TIMEOUT 5000

static const char* gw_error(int e)
{
    switch (e)
    {
        case ACK_OKAY:           return "OK";
        case ACK_BAD_COMMAND:    return "Bad command";
        case ACK_NO_INDEX:       return "No index";
        case ACK_NO_TRK0:        return "No track 0";
        case ACK_FLUX_OVERFLOW:  return "Overflow";
        case ACK_FLUX_UNDERFLOW: return "Underflow";
        case ACK_WRPROT:         return "Write protected";
        case ACK_NO_UNIT:        return "No unit";
        case ACK_NO_BUS:         return "No bus";
        case ACK_BAD_UNIT:       return "Invalid unit";
        case ACK_BAD_PIN:        return "Invalid pin";
        case ACK_BAD_CYLINDER:   return "Invalid cylinder";
        default:                 return "Unknown error";
    }
}

class GreaseWeazleUsb : public USB
{
    uint8_t _readbuffer[4096];
    int _readbuffer_ptr = 0;
    int _readbuffer_fill = 0;

    void read_bytes(uint8_t* buffer, int len)
    {
        while (len > 0)
        {
            if (_readbuffer_ptr < _readbuffer_fill)
            {
                int buffered = std::min(len, _readbuffer_fill - _readbuffer_ptr);
                memcpy(buffer, _readbuffer + _readbuffer_ptr, buffered);
                _readbuffer_ptr += buffered;
                buffer += buffered;
                len -= buffered;
            }

            if (len == 0)
                break;

            int actual;
            int rc = libusb_bulk_transfer(_device, EP_IN,
                _readbuffer, sizeof(_readbuffer),
                &actual, TIMEOUT);
            if (rc < 0)
                Error() << "failed to receive command reply: " << usberror(rc);

            _readbuffer_fill = actual;
            _readbuffer_ptr = 0;
        }
    }

    void read_bytes(Bytes& bytes)
    {
        read_bytes(bytes.begin(), bytes.size());
    }

    Bytes read_bytes(unsigned len)
    {
        Bytes b(len);
        read_bytes(b);
        return b;
    }

    void write_bytes(const uint8_t* buffer, int len)
    {
        while (len > 0)
        {
            int actual;
            int rc = libusb_bulk_transfer(_device, EP_OUT, (uint8_t*)buffer, len, &actual, 0);
            if (rc < 0)
                Error() << "failed to send command: " << usberror(rc);

            buffer += actual;
            len -= actual;
        }
    }

    void write_bytes(const Bytes& bytes)
    {
        write_bytes(bytes.cbegin(), bytes.size());
    }

    void do_command(const Bytes& command)
    {
        write_bytes(command);

        uint8_t buffer[2];
        read_bytes(buffer, sizeof(buffer));

        if (buffer[0] != command[0])
            Error() << fmt::format("command returned garbage (0x{:x} != 0x{:x} with status 0x{:x})",
                buffer[0], command[0], buffer[1]);
        if (buffer[1])
            Error() << fmt::format("GreaseWeazle error: {}", gw_error(buffer[1]));
    }

public:
    GreaseWeazleUsb(libusb_device_handle* device)
    {
        _device = device;

        /* Configure the device. */

        int i;
        int cfg = -1;
        libusb_get_configuration(_device, &cfg);
        if (cfg != 1)
        {
            i = libusb_set_configuration(_device, 1);
            if (i < 0)
                Error() << "the GreaseWeazle would not accept configuration: " << usberror(i);
        }

        /* Detach the existing kernel serial port driver, if there is one, and claim it ourselves. */

        for (int i = 0; i < 2; i++)
        {
            if (libusb_kernel_driver_active(_device, i))
                libusb_detach_kernel_driver(_device, i);
            int rc = libusb_claim_interface(_device, i);
            if (rc < 0)
                Error() << "unable to claim interface: " << libusb_error_name(rc);
        }

        int version = getVersion();
        if (version != GREASEWEAZLE_VERSION)
            Error() << "your GreaseWeazle firmware is at version " << version
                    << " but the client is for version " << GREASEWEAZLE_VERSION
                    << "; please upgrade";

        /* Configure the hardware. */

        do_command({ CMD_SET_BUS_TYPE, 3, BUS_IBMPC });
    }

    int getVersion()
    {
        do_command({ CMD_GET_INFO, 3, GETINFO_FIRMWARE });

        Bytes response = read_bytes(32);
        ByteReader br(response);
        return br.read_be16();
    }

    void recalibrate()
    { Error() << "unsupported operation"; }
    
    void seek(int track)
    {
        do_command({ CMD_SEEK, 3, (uint8_t)track });
    }
    
    nanoseconds_t getRotationalPeriod()
    { Error() << "unsupported operation"; }
    
    void testBulkWrite()
    {
        const int LEN = 10*1024*1024;
        Bytes cmd(6);
        ByteWriter bw(cmd);
        bw.write_8(CMD_SINK_BYTES);
        bw.write_8(cmd.size());
        bw.write_le32(LEN);
        do_command(cmd);

        Bytes junk(LEN);
		double start_time = getCurrentTime();
        write_bytes(LEN);
        read_bytes(1);
		double elapsed_time = getCurrentTime() - start_time;

		std::cout << "Transferred "
				  << LEN
				  << " bytes from PC -> GreaseWeazle in "
				  << int(elapsed_time * 1000.0)
				  << " ms ("
				  << int((LEN / 1024.0) / elapsed_time)
				  << " kB/s)"
				  << std::endl;
    }
    
    void testBulkRead()
    {
        const int LEN = 10*1024*1024;
        Bytes cmd(6);
        ByteWriter bw(cmd);
        bw.write_8(CMD_SOURCE_BYTES);
        bw.write_8(cmd.size());
        bw.write_le32(LEN);
        do_command(cmd);

		double start_time = getCurrentTime();
        read_bytes(LEN);
		double elapsed_time = getCurrentTime() - start_time;

		std::cout << "Transferred "
				  << LEN
				  << " bytes from GreaseWeazle -> PC in "
				  << int(elapsed_time * 1000.0)
				  << " ms ("
				  << int((LEN / 1024.0) / elapsed_time)
				  << " kB/s)"
				  << std::endl;
    }
    
    Bytes read(int side, bool synced, nanoseconds_t readTime)
    { Error() << "unsupported operation"; }
    
    void write(int side, const Bytes& bytes)
    { Error() << "unsupported operation"; }
    
    void erase(int side)
    { Error() << "unsupported operation"; }
    
    void setDrive(int drive, bool high_density, int index_mode)
    {
        do_command({ CMD_SELECT, 3, (uint8_t)drive });
        do_command({ CMD_MOTOR, 4, (uint8_t)drive, 1 });
    }

    void measureVoltages(struct voltages_frame* voltages)
    { Error() << "unsupported operation on the GreaseWeazle"; }
};

USB* createGreaseWeazleUsb(libusb_device_handle* device)
{
    return new GreaseWeazleUsb(device);
}

// vim: sw=4 ts=4 et

