#include "globals.h"
#include "usb.h"
#include "protocol.h"
#include "fluxmap.h"
#include "bytes.h"
#include <libusb.h>
#include "fmt/format.h"

static USB* usb = NULL;

USB::~USB()
{}

std::string USB::usberror(int i)
{
    return libusb_strerror((libusb_error) i);
}

USB& getUsb()
{
	if (!usb)
		usb = createFluxengineUsb();

	return *usb;
}

