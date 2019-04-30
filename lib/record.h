#ifndef RECORD_H
#define RECORD_H

#include "fluxmap.h"

class RawRecord
{
public:
	RawRecord() {}

	Fluxmap::Position position;
	nanoseconds_t clock = 0;
    int physicalTrack = 0;
    int physicalSide = 0;
	Bytes data;
};

#endif

