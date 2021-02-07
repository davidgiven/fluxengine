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
	virtual nanoseconds_t getRotationalPeriod(int hardSectorCount) = 0;
	virtual void testBulkWrite() = 0;
	virtual void testBulkRead() = 0;
	virtual Bytes read(int side, bool synced, nanoseconds_t readTime,
	                   nanoseconds_t hardSectorThreshold) = 0;
	virtual void write(int side, const Bytes& bytes,
	                   nanoseconds_t hardSectorThreshold) = 0;
	virtual void erase(int side, nanoseconds_t hardSectorThreshold) = 0;
	virtual void setDrive(int drive, bool high_density, int index_mode,
		int step_interval_time, int step_settling_time, bool double_step) = 0;
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

static inline void usbErase(int side, nanoseconds_t hardSectorThreshold)
{ getUsb().erase(side, hardSectorThreshold); }

static inline nanoseconds_t usbGetRotationalPeriod(int hardSectorCount)
{ return getUsb().getRotationalPeriod(hardSectorCount); }

static inline Bytes usbRead(int side, bool synced, nanoseconds_t readTime,
                            nanoseconds_t hardSectorThreshold)
{ return getUsb().read(side, synced, readTime, hardSectorThreshold); }

static inline void usbWrite(int side, const Bytes& bytes,
                            nanoseconds_t hardSectorThreshold)
{ getUsb().write(side, bytes, hardSectorThreshold); }

static inline void usbSetDrive(int drive, bool high_density, int index_mode,
	int step_interval_time, int step_settling_time, bool double_step)
{ getUsb().setDrive(drive, high_density, index_mode,
	step_interval_time, step_settling_time, double_step); }

static inline void usbMeasureVoltages(struct voltages_frame* voltages)
{ getUsb().measureVoltages(voltages); }

#endif
