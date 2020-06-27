#ifndef RECORD_H
#define RECORD_H

#include "fluxmap.h"

class RawRecord
{
public:
	RawRecord() {}

	Fluxmap::Position position;
	std::vector<nanoseconds_t> intervals;
    int physicalTrack = 0;
    int physicalSide = 0;
	Bytes data;
};

#endif

