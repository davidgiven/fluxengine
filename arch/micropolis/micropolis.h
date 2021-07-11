#ifndef MICROPOLIS_H
#define MICROPOLIS_H

#define MICROPOLIS_ENCODED_SECTOR_SIZE (1+2+266+6)

class Sector;
class Fluxmap;
class MicropolisDecoderProto;

class MicropolisDecoder : public AbstractDecoder
{
public:
	MicropolisDecoder(const MicropolisDecoderProto&) {}
	virtual ~MicropolisDecoder() {}

	RecordType advanceToNextRecord();
	void decodeSectorRecord();
};

extern std::unique_ptr<AbstractDecoder> createMicropolisDecoder(const DecoderProto& config);

#endif
