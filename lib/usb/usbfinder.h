#ifndef USBSERIAL_H
#define USBSERIAL_H

#include "libusbp_config.h"
#include "libusbp.hpp"

enum DeviceType
{
    DEVICE_FLUXENGINE,
    DEVICE_GREASEWEAZLE,
    DEVICE_APPLESAUCE,
};

extern std::string getDeviceName(DeviceType type);

struct CandidateDevice
{
    DeviceType type;
    libusbp::device device;
    uint32_t id;
    std::string serial;
    std::string serialPort;
};

extern std::vector<std::shared_ptr<CandidateDevice>> findUsbDevices();

#endif
