#ifndef ZILOGMCZ_H
#define ZILOGMCZ_H

#define MICROPOLIS_ENCODED_SECTOR_SIZE (1+2+266+6)

class Sector;
class Fluxmap;

class MicropolisDecoder : public AbstractDecoder
{
public:
	virtual ~MicropolisDecoder() {}

	RecordType advanceToNextRecord();
	void decodeSectorRecord();
};

#endif
