#ifndef USB_H
#define USB_H

extern int usbGetVersion();
extern void usbSeek(uint8_t track);
extern nanoseconds_t usbGetRotationalPeriod();
extern void usbTestBulkTransport();

#endif
