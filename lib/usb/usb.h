#ifndef USB_H
#define USB_H

#include "bytes.h"
#include "flags.h"

class Fluxmap;
class libusb_device_handle;

class USB
{
public:
	virtual ~USB();

	virtual int getVersion() = 0;
	virtual void recalibrate() = 0;
	virtual void seek(int track) = 0;
	virtual nanoseconds_t getRotationalPeriod() = 0;
	virtual void testBulkWrite() = 0;
	virtual void testBulkRead() = 0;
	virtual Bytes read(int side, bool synced, nanoseconds_t readTime) = 0;
	virtual void write(int side, const Bytes& bytes) = 0;
	virtual void erase(int side) = 0;
	virtual void setDrive(int drive, bool high_density, int index_mode) = 0;
	virtual void measureVoltages(struct voltages_frame* voltages) = 0;

protected:
	std::string usberror(int i);

	libusb_device_handle* _device;
};

extern FlagGroup usbFlags;
extern USB& getUsb();

extern USB* createFluxengineUsb(libusb_device_handle* device);
extern USB* createGreaseWeazleUsb(libusb_device_handle* device);

static inline int usbGetVersion()     { return getUsb().getVersion(); }
static inline void usbRecalibrate()   { getUsb().recalibrate(); }
static inline void usbSeek(int track) { getUsb().seek(track); }
static inline void usbTestBulkWrite() { getUsb().testBulkWrite(); }
static inline void usbTestBulkRead()  { getUsb().testBulkRead(); }
static inline void usbErase(int side) { getUsb().erase(side); }

static inline nanoseconds_t usbGetRotationalPeriod()
{ return getUsb().getRotationalPeriod(); }

static inline Bytes usbRead(int side, bool synced, nanoseconds_t readTime)
{ return getUsb().read(side, synced, readTime); }

static inline void usbWrite(int side, const Bytes& bytes)
{ getUsb().write(side, bytes); }

static inline void usbSetDrive(int drive, bool high_density, int index_mode)
{ getUsb().setDrive(drive, high_density, index_mode); }

static inline void usbMeasureVoltages(struct voltages_frame* voltages)
{ getUsb().measureVoltages(voltages); }

#endif
