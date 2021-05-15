#ifndef MX_H
#define MX_H

#include "decoders/decoders.h"

class MxInputProto;

class MxDecoder : public AbstractDecoder
{
public:
	MxDecoder(const MxInputProto&) {}
    virtual ~MxDecoder() {}

    void beginTrack();
    RecordType advanceToNextRecord();
    void decodeSectorRecord();

private:
    nanoseconds_t _clock;
    int _currentSector;
    int _logicalTrack;
};

#endif
