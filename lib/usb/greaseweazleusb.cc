#include "globals.h"
#include "usb.h"
#include "protocol.h"
#include "fluxmap.h"
#include "bytes.h"
#include <libusb.h>
#include "fmt/format.h"

class GreaseWeazleUsb : public USB
{
public:
	GreaseWeazleUsb(libusb_device_handle* device) {}
	~GreaseWeazleUsb() {}

	int getVersion()
	{ Error() << "unsupported operation"; }

	void recalibrate()
	{ Error() << "unsupported operation"; }
	
	void seek(int track)
	{ Error() << "unsupported operation"; }
	
	nanoseconds_t getRotationalPeriod()
	{ Error() << "unsupported operation"; }
	
	void testBulkWrite()
	{ Error() << "unsupported operation"; }
	
	void testBulkRead()
	{ Error() << "unsupported operation"; }
	
	Bytes read(int side, bool synced, nanoseconds_t readTime)
	{ Error() << "unsupported operation"; }
	
	void write(int side, const Bytes& bytes)
	{ Error() << "unsupported operation"; }
	
	void erase(int side)
	{ Error() << "unsupported operation"; }
	
	void setDrive(int drive, bool high_density, int index_mode)
	{ Error() << "unsupported operation"; }

	void measureVoltages(struct voltages_frame* voltages)
	{ Error() << "unsupported operation"; }
};

USB* createGreaseWeazleUsb(libusb_device_handle* device)
{
	return new GreaseWeazleUsb(device);
}


