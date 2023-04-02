#ifndef USBSERIAL_H
#define USBSERIAL_H

#include "libusbp_config.h"
#include "libusbp.hpp"

struct CandidateDevice
{
	libusbp::device device;
	uint32_t id;
	std::string serial;
	std::string serialPort;
};

extern std::vector<std::shared_ptr<CandidateDevice>> findUsbDevices();

#endif

