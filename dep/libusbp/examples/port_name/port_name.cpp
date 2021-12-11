// This example shows how to get the name of a USB serial port (e.g. "COM6")
// for a single USB device.

#include <libusbp.hpp>
#include <iostream>
#include <iomanip>

const uint16_t vendor_id = 0x1FFB;
const uint16_t product_id = 0xDA01;
const uint8_t interface_number = 2;
const bool composite = true;

int main_with_exceptions()
{
    libusbp::device device = libusbp::find_device_with_vid_pid(vendor_id, product_id);
    if (!device)
    {
        std::cerr << "Device not found." << std::endl;
        return 1;
    }

    libusbp::serial_port port(device, interface_number, composite);
    std::string port_name = port.get_name();
    std::cout << port_name << std::endl;

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
