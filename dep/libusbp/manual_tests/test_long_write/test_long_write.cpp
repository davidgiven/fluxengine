/* Tests what happens if we do a synchronous write that takes a long time to
 * complete.  This can be used to check that pressing Ctrl+C is able to
 * interrupt the write. */

#include <libusbp.hpp>
#include <stdio.h>
#include <iostream>

const uint16_t vendor_id = 0x1FFB;
const uint16_t product_id = 0xDA01;
const uint8_t interface_number = 0;
const bool composite = true;
const uint8_t endpoint_address = 0x03;
const size_t packet_size = 32;
const size_t transfer_size = packet_size * 4;

void long_write(libusbp::generic_handle & handle)
{
    // First packet causes a delay of 4 seconds (0x0FA0 ms)
    uint8_t buffer[transfer_size] = { 0xDE, 0xA0, 0x0F };
    size_t transferred;

    printf("Writing...\n");
    fflush(stdout);
    handle.write_pipe(endpoint_address, buffer, sizeof(buffer), &transferred);
    if (transferred == transfer_size)
    {
        printf("Transfer successful.\n");
    }
    else
    {
        printf("Transferred only %d bytes out of %d.\n",
            (unsigned int)transferred, (unsigned int)transfer_size);
    }
}

int main_with_exceptions()
{
    libusbp::device device = libusbp::find_device_with_vid_pid(
        vendor_id, product_id);
    if (!device)
    {
        std::cerr << "Device not found." << std::endl;
        return 1;
    }

    libusbp::generic_interface gi(device, interface_number, composite);
    libusbp::generic_handle handle(gi);

    long_write(handle);

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
