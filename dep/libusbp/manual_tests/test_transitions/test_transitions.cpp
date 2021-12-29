/* This program helps us test the transitions that a USB device goes through as
 * it gets connected or disconnected from a computer.  It helps us identify
 * errors that might occur so we can assign code to them such as
 * LIBUSBP_ERROR_NOT_READY. */

#include <libusbp.hpp>
#include <iostream>
#include <iomanip>
#include <chrono>
#ifdef _MSC_VER
#define usleep(x) Sleep(((x) + 999) / 1000)
#else
#include <unistd.h>
#endif

#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ == 4 && __GNUC_MINOR__ <= 6
typedef std::chrono::monotonic_clock clock_type;
#else
typedef std::chrono::steady_clock clock_type;
#endif

std::ostream & log()
{
    return std::cout
        << clock_type::now().time_since_epoch().count()
        << ": ";
}

void check_test_device_a(libusbp::device device)
{
    static std::string current_status;
    std::string status;

    if (device)
    {
        status = "Found " + device.get_serial_number() + ".";

        // Try to connect to the generic interface.
        try
        {
            libusbp::generic_interface gi(device, 0, true);
            libusbp::generic_handle handle(gi);
            status += "  Interface 0 works.";
        }
        catch(const libusbp::error & error)
        {
            status += "  Interface 0 error: " + error.message();
            if (!error.has_code(LIBUSBP_ERROR_NOT_READY))
            {
                status += "  Lacks code LIBUSBP_ERROR_NOT_READY!";
            }
        }
    }
    else
    {
        status = "Not found.";
    }

    if (current_status != status)
    {
        log() << "Test device A: " << status << std::endl;
        current_status = status;
    }
}

int main_with_exceptions()
{
    std::cout
        << "Clock tick period: "
        << clock_type::period::num
        << "/"
        << clock_type::period::den
        << " seconds" << std::endl;

    while(1)
    {
        check_test_device_a(libusbp::find_device_with_vid_pid(0x1FFB, 0xDA01));
        usleep(10);
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
