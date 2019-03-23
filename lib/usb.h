#ifndef USB_H
#define USB_H

class Fluxmap;

extern int usbGetVersion();
extern void usbRecalibrate();
extern void usbSeek(int track);
extern nanoseconds_t usbGetRotationalPeriod();
extern void usbTestBulkTransport();
extern std::unique_ptr<Fluxmap> usbRead(int side, int revolutions);
extern void usbWrite(int side, const Fluxmap& fluxmap);
extern void usbErase(int side);
extern void usbSetDrive(int drive, bool high_density);

#endif
