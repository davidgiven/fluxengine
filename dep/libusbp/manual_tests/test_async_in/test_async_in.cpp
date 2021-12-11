/* Tests that libusbp is capable of reliably reading data from an IN endpoint on
 * every frame using asynchronous transfers.  It prints to the standard output
 * as it successfully receives data, and prints to the standard error if there
 * was a gap in the data (a USB frame where the device did not generate a new
 * packet for the host).
 *
 * You can also check the CPU usage while running this function to make
 * sure libusbp is not doing anything too inefficient.
 */

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
const size_t packet_size = 5;
const size_t transfer_size = packet_size;
const size_t transfer_count = 250;

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

    uint8_t last_f = 0;

    uint32_t finish_count = 0;
    while(true)
    {
        uint8_t buffer[transfer_size];
        size_t transferred;
        libusbp::error transfer_error;
        while(pipe.handle_finished_transfer(buffer, &transferred, &transfer_error))
        {
            if (transfer_error)
            {
                fprintf(stderr, "Transfer error.\n");
                throw transfer_error;
            }

	    if (transferred != transfer_size)
	    {
	        fprintf(stderr, "Got %d bytes instead of %d.\n",
			(int)transferred, (int)transfer_size);
	    }

	    uint8_t f = buffer[0];
	    if (f != (uint8_t)(last_f + transfer_size/packet_size))
	    {
               // If this happens, it indicates there was a USB frame where the
               // device did not generate a new packet for the host, which is
               // bad.  However, you should expect to see a few of these at the
               // very beginning of the test because there will be some old
               // packets queued up in the device from earlier, and because
               // last_f always starts at 0.
               fprintf(stderr, "Frame number gap: %d to %d\n", last_f, f);
	    }
	    last_f = f;

            if ((++finish_count % 4096) == 0)
            {
                printf("Another 4096 transfers done.\n");
                fflush(stdout);
            }
        }

        pipe.handle_events();
        usleep(20000);
    }
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
