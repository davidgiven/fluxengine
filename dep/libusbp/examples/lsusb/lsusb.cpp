/* This example shows how to gather information about the USB devices connected
 * to the system and print it. */

#include <libusbp.hpp>
#include <iostream>
#include <iomanip>

std::string serial_number_or_default(const libusbp::device & device,
    const std::string & def)
{
    try
    {
        return device.get_serial_number();
    }
    catch (const libusbp::error & error)
    {
        if (error.has_code(LIBUSBP_ERROR_NO_SERIAL_NUMBER))
        {
            return def;
        }
        throw;
    }
}

void print_device(libusbp::device & device)
{
    uint16_t vendor_id = device.get_vendor_id();
    uint16_t product_id = device.get_product_id();
    uint16_t revision = device.get_revision();
    std::string serial_number = serial_number_or_default(device, "-");
    std::string os_id = device.get_os_id();

    // Note: The serial number might have spaces in it, so it should be the last
    // field to avoid confusing programs that are looking for a field after the
    // serial number.

    std::ios::fmtflags flags(std::cout.flags());
    std::cout
        << std::hex << std::setfill('0') << std::right
        << std::setw(4) << vendor_id
        << ':'
        << std::setw(4) << product_id
        << ' '
        << std::setfill(' ') << std::setw(2) << (revision >> 8)
        << '.'
        << std::setfill('0') << std::setw(2) << (revision & 0xFF)
        << ' '
        << os_id
        << ' '
        << std::setfill(' ') << std::left << serial_number
        << std::endl;
    std::cout.flags(flags);
}

int main_with_exceptions()
{
    std::vector<libusbp::device> list = libusbp::list_connected_devices();
    for (auto it = list.begin(); it != list.end(); ++it)
    {
        print_device(*it);
    }
    return 0;
}

int main(int argc, char ** argv)
{
    // Suppress unused parameter warnings.
    (void)argc;
    (void)argv;

    try
    {
        return main_with_exceptions();
    }
    catch(const std::exception & error)
    {
        std::cerr << "Error: " << error.what() << std::endl;
    }
    return 1;
}
