/* Tests what happens if we do a synchronous read from an IN endpoint that
 * takes a long time to complete.  This can be used to check that pressing
 * Ctrl+C is able to interrupt the read. */

#include <libusbp.hpp>
#include <stdio.h>
#include <iostream>

const uint16_t vendor_id = 0x1FFB;
const uint16_t product_id = 0xDA01;
const uint8_t interface_number = 0;
const bool composite = true;
const uint8_t endpoint_address = 0x82;
const size_t packet_size = 5;
const size_t transfer_size = packet_size * 10000;

void long_read(libusbp::generic_handle & handle)
{
    uint8_t buffer[transfer_size];
    size_t transferred;

    printf("Reading %d bytes...\n", (unsigned int)transfer_size);
    fflush(stdout);
    handle.read_pipe(endpoint_address, buffer, sizeof(buffer), &transferred);
    if (transferred == transfer_size)
    {
        printf("Transfer successful.\n");
    }
    else
    {
        printf("Transferred only %d bytes out of %d.\n",
            (unsigned int)transferred, (unsigned int)transfer_size);
    }
    fflush(stdout);
}

void long_control_read(libusbp::generic_handle & handle)
{
    uint8_t buffer[5];
    size_t transferred;

    printf("Performing a slow control read...\n");
    fflush(stdout);
    handle.control_transfer(0xC0, 0x91, 10000, 5, buffer, sizeof(buffer), &transferred);
    if (transferred == 5)
    {
        printf("Control read successful.\n");
    }
    else
    {
        printf("Transferred only %d bytes out of %d.\n",
            (unsigned int)transferred, (unsigned int)sizeof(buffer));
    }
    fflush(stdout);
}

int main_with_exceptions()
{
    libusbp::device device = libusbp::find_device_with_vid_pid(vendor_id, product_id);
    if (!device)
    {
        std::cerr << "Device not found." << std::endl;
        return 1;
    }

    libusbp::generic_interface gi(device, interface_number, composite);
    libusbp::generic_handle handle(gi);

    long_read(handle);
    long_control_read(handle);

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
