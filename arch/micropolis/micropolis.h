#ifndef MICROPOLIS_H
#define MICROPOLIS_H

#define MICROPOLIS_ENCODED_SECTOR_SIZE (1+2+266+6)

class Sector;
class Fluxmap;
class MicropolisInputProto;

class MicropolisDecoder : public AbstractDecoder
{
public:
	MicropolisDecoder(const MicropolisInputProto&) {}
	virtual ~MicropolisDecoder() {}

	RecordType advanceToNextRecord();
	void decodeSectorRecord();
};

#endif
