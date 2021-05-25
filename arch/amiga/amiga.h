#ifndef AMIGA_H
#define AMIGA_H

#include "encoders/encoders.h"

#define AMIGA_SECTOR_RECORD 0xaaaa44894489LL

#define AMIGA_TRACKS_PER_DISK 80
#define AMIGA_SECTORS_PER_TRACK 11
#define AMIGA_RECORD_SIZE 0x21f

class Sector;
class Fluxmap;
class SectorSet;
class AmigaDecoderProto;
class AmigaEncoderProto;
class DisassemblingGeometryMapper;

class AmigaDecoder : public AbstractDecoder
{
public:
	AmigaDecoder(const AmigaDecoderProto&) {}
    virtual ~AmigaDecoder() {}

    RecordType advanceToNextRecord();
    void decodeSectorRecord();

	std::set<unsigned> requiredSectors(Track& track) const;
};

class AmigaEncoder : public AbstractEncoder
{
public:
	AmigaEncoder(const AmigaEncoderProto& config, const DisassemblingGeometryMapper& mapper):
		_config(config),
		_mapper(mapper)
	{}
	virtual ~AmigaEncoder() {}

public:
    std::unique_ptr<Fluxmap> encode(int physicalTrack, int physicalSide);

private:
	const AmigaEncoderProto& _config;
	const DisassemblingGeometryMapper& _mapper;
};

extern uint32_t amigaChecksum(const Bytes& bytes);
extern Bytes amigaInterleave(const Bytes& input);
extern Bytes amigaDeinterleave(const uint8_t*& input, size_t len);
extern Bytes amigaDeinterleave(const Bytes& input);

#endif
