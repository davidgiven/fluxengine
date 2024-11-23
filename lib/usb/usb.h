#ifndef USB_H
#define USB_H

#include "lib/core/bytes.h"
#include "lib/config/flags.h"

class Fluxmap;
class GreaseweazleProto;
class ApplesauceProto;
namespace libusbp
{
    class device;
}

class USB
{
public:
    virtual ~USB();

    virtual void recalibrate()
    {
        seek(0);
    };
    virtual void seek(int track) = 0;
    virtual nanoseconds_t getRotationalPeriod(int hardSectorCount) = 0;
    virtual void testBulkWrite() = 0;
    virtual void testBulkRead() = 0;
    virtual Bytes read(int side,
        bool synced,
        nanoseconds_t readTime,
        nanoseconds_t hardSectorThreshold) = 0;
    virtual void write(
        int side, const Bytes& bytes, nanoseconds_t hardSectorThreshold) = 0;
    virtual void erase(int side, nanoseconds_t hardSectorThreshold) = 0;
    virtual void setDrive(int drive, bool high_density, int index_mode) = 0;
    virtual void measureVoltages(struct voltages_frame* voltages) = 0;

protected:
    std::string usberror(int i);
};

extern USB& getUsb();

extern USB* createFluxengineUsb(libusbp::device& device);
extern USB* createGreaseweazleUsb(
    const std::string& serialPort, const GreaseweazleProto& config);
extern USB* createApplesauceUsb(
    const std::string& serialPort, const ApplesauceProto& config);

static inline void usbRecalibrate()
{
    getUsb().recalibrate();
}
static inline void usbSeek(int track)
{
    getUsb().seek(track);
}
static inline void usbTestBulkWrite()
{
    getUsb().testBulkWrite();
}
static inline void usbTestBulkRead()
{
    getUsb().testBulkRead();
}

static inline void usbErase(int side, nanoseconds_t hardSectorThreshold)
{
    getUsb().erase(side, hardSectorThreshold);
}

static inline nanoseconds_t usbGetRotationalPeriod(int hardSectorCount)
{
    return getUsb().getRotationalPeriod(hardSectorCount);
}

static inline Bytes usbRead(int side,
    bool synced,
    nanoseconds_t readTime,
    nanoseconds_t hardSectorThreshold)
{
    return getUsb().read(side, synced, readTime, hardSectorThreshold);
}

static inline void usbWrite(
    int side, const Bytes& bytes, nanoseconds_t hardSectorThreshold)
{
    getUsb().write(side, bytes, hardSectorThreshold);
}

static inline void usbSetDrive(int drive, bool high_density, int index_mode)
{
    getUsb().setDrive(drive, high_density, index_mode);
}

static inline void usbMeasureVoltages(struct voltages_frame* voltages)
{
    getUsb().measureVoltages(voltages);
}

#endif
