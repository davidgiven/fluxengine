#include "globals.h"
#include <libusb.h>

static libusb_device_handle* device;

void usb_init(void)
{
	int i = libusb_init(NULL);
    if (i < 0)
        error("could not start libusb: %s", libusb_strerror(i));

    device = libusb_open_device_with_vid_pid(NULL, FLUXENGINE_VID, FLUXENGINE_PID);
    if (!device)
		error("cannot find FluxEngine (is it plugged in?)");
    
    int cfg = -1;
    libusb_get_configuration(device, &cfg);
    if (cfg != 1)
    {
        i = libusb_set_configuration(device, 1);
        if (i < 0)
            error("FluxEngine would not accept configuration: %s", libusb_strerror(i));
    }

    i = libusb_claim_interface(device, 0);
    if (i < 0)
        error("could not claim interface: %s", libusb_strerror(i));        
}

void usb_send(void* ptr, int len)
{
    int i = libusb_bulk_transfer(device, FLUXENGINE_DATA_OUT_EP,
        ptr, len, &len, 0);
    if (i < 0)
        error("failed to send request: %s", libusb_strerror(i));
}
