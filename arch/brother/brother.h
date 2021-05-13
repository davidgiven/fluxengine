#ifndef BROTHER_H
#define BROTHER_H

/* Brother word processor format (or at least, one of them) */

#define BROTHER_SECTOR_RECORD            0xFFFFFD57
#define BROTHER_DATA_RECORD              0xFFFFFDDB
#define BROTHER_DATA_RECORD_PAYLOAD      256
#define BROTHER_DATA_RECORD_CHECKSUM     3
#define BROTHER_DATA_RECORD_ENCODED_SIZE 415

#define BROTHER_TRACKS_PER_240KB_DISK    78
#define BROTHER_TRACKS_PER_120KB_DISK    39
#define BROTHER_SECTORS_PER_TRACK        12

class Sector;
class SectorSet;
class Fluxmap;
class BrotherInputProto;
class BrotherOutputProto;

class BrotherDecoder : public AbstractDecoder
{
public:
    BrotherDecoder(const BrotherInputProto& config) {}
    virtual ~BrotherDecoder() {}

    RecordType advanceToNextRecord();
    void decodeSectorRecord();
    void decodeDataRecord();
};

class BrotherEncoder : public AbstractEncoder
{
public:
	BrotherEncoder(const BrotherOutputProto& config):
		_config(config)
	{}

	virtual ~BrotherEncoder() {}

private:
	const BrotherOutputProto& _config;

public:
    std::unique_ptr<Fluxmap> encode(int physicalTrack, int physicalSide, const SectorSet& allSectors);
};

#endif
