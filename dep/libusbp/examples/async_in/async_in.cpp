/* This example shows how to read a constant stream of data from an IN endpoint.
 * By queueing up a large number of asynchronous transfers, we ensure that the
 * USB host controller is busy, and that it will retrieve data from the endpoint
 * as fast as possible for an indefinite period of time.
 *
 * This example is designed to connect to Test Device A and read ADC data from
 * endpoint 0x82. */

#include <libusbp.hpp>
#include <stdio.h>
#include <iostream>
#ifdef _MSC_VER
#define usleep(x) Sleep(((x) + 999) / 1000)
#else
#include <unistd.h>
#endif

const uint16_t vendor_id = 0x1FFB;
const uint16_t product_id = 0xDA01;
const uint8_t interface_number = 0;
const bool composite = true;
const uint8_t endpoint_address = 0x82;
const size_t transfer_size = 5;
const size_t transfer_count = 250;

// Prints the data in the given buffer to the standard output in HEX.
void print_data(uint8_t * buffer, size_t size)
{
    for (uint8_t i = 0; i < size; i++)
    {
        printf("%02x", buffer[i]);
        if (i < size - 1) { putchar(' '); }
    }
    printf("\n");
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
    libusbp::async_in_pipe pipe = handle.open_async_in_pipe(endpoint_address);
    pipe.allocate_transfers(transfer_count, transfer_size);

    pipe.start_endless_transfers();

    while(true)
    {
        uint8_t buffer[transfer_size];
        size_t transferred;
        libusbp::error transfer_error;
        while(pipe.handle_finished_transfer(buffer, &transferred, &transfer_error))
        {
            if (transfer_error) { throw transfer_error; }
            print_data(buffer, transferred);
        }

        pipe.handle_events();
        usleep(500);
    }

    // Note that closing an async_in_pipe cleanly without causing memory leaks
    // can be difficult.  For more information, see the documentation of
    // libusbp_async_in_pipe_close in libusbp.h.  In this example, we don't
    // worry about it because we are only ever creating one pipe, so the amount
    // of the memory leak is bounded.
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
