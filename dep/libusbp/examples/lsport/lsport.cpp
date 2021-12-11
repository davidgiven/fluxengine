// This example prints the names of all USB serial ports along with information
// about the USB devices they belong to.
//
// For each USB device, it prints the USB vendor ID, product ID, and serial
// number on a line.  Then, on the following lines, it prints any serial port
// names it found, sorted by interface number, ascending.
//
// Note: This example is slow and ugly because libusbp does not yet have
// built-in support for listing serial ports; it only has support for finding
// a serial port if you already know what USB device it is connected to and what
// interface you expect the port to be on.  This might be improved in the future.

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

bool try_print_port_name(const libusbp::device & device,
  uint8_t interface_number, bool composite)
{
    std::string port_name;
    try
    {
        libusbp::serial_port port(device, interface_number, composite);
        port_name = port.get_name();
    }
    catch (const libusbp::error & error)
    {
        return false;
    }
    std::cout << "  " << port_name << std::endl;
    return true;
}

int main_with_exceptions()
{
    auto devices = libusbp::list_connected_devices();
    for (const libusbp::device & device : devices)
    {
      bool success = false;

      // Print the USB device info.
      uint16_t vendor_id = device.get_vendor_id();
      uint16_t product_id = device.get_product_id();
      std::string serial_number = serial_number_or_default(device, "-");
      std::ios::fmtflags flags(std::cout.flags());
      std::cout
          << std::hex << std::setfill('0') << std::right
          << std::setw(4) << vendor_id
          << ':'
          << std::setw(4) << product_id
          << ' '
          << std::setfill(' ') << std::left << serial_number
          << std::endl;
      std::cout.flags(flags);

      // First, assume the device is composite and try the first 16 interfaces.
      // Most devices don't have more interfaces than that.  Trying all 255 possible
      // interfaces slows the program down noticeably.  This issue could be fixed if
      // we added better serial port enumeration support to libusbp.
      for (uint32_t i = 0; i < 16; i++)
      {
        success = try_print_port_name(device, i, true) || success;
      }

      // Try to find a port assuming the device is non-composite.  Only do so if
      // no ports were found earlier, to help avoid printing the same port twice.
      if (!success)
      {
        try_print_port_name(device, 0, false);
      }
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
