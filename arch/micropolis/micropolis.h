#ifndef MICROPOLIS_H
#define MICROPOLIS_H

#include "decoders/decoders.h"
#include "encoders/encoders.h"

#define MICROPOLIS_ENCODED_SECTOR_SIZE (1+2+266+6)

class MicropolisDecoder : public AbstractDecoder
{
public:
	virtual ~MicropolisDecoder() {}

	RecordType advanceToNextRecord();
	void decodeSectorRecord();
};

class MicropolisEncoder : public AbstractEncoder
{
public:
	virtual ~MicropolisEncoder() {}

	std::unique_ptr<Fluxmap> encode(int physicalTrack, int physicalSide, const SectorSet& allSectors);
};

extern FlagGroup micropolisEncoderFlags;

extern uint8_t micropolisChecksum(const Bytes& bytes);

#endif
