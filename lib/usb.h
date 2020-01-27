#ifndef USB_H
#define USB_H

class Fluxmap;
class Bytes;

extern int usbGetVersion();
extern void usbRecalibrate();
extern void usbSeek(int track);
extern nanoseconds_t usbGetRotationalPeriod();
extern void usbTestBulkTransport();
extern Bytes usbRead(int side, bool synced, nanoseconds_t readTime);
extern void usbWrite(int side, const Bytes& bytes);
extern void usbErase(int side);
extern void usbSetDrive(int drive, bool high_density, int index_mode);
extern void usbMeasureVoltages(struct voltages_frame* voltages);

#endif
