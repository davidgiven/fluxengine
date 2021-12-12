#ifndef USBSERIAL_H
#define USBSERIAL_H

#include "libusbp.hpp"

struct CandidateDevice
{
	libusbp::device device;
	uint32_t id;
	std::string serial;
};

extern std::vector<std::unique_ptr<CandidateDevice>> findUsbDevices(uint32_t id);

#endif

