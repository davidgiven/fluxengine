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
class Fluxmap;

class BrotherDecoder : public AbstractDecoder
{
public:
    virtual ~BrotherDecoder() {}

	int getDecoderBands() const { return 2; }
	bool isInterleaved() const { return false; }

    RecordType advanceToNextRecord();
    void decodeSectorRecord();
    void decodeDataRecord();
};

class BrotherEncoder : public AbstractEncoder
{
public:
	BrotherEncoder(int format, int bias):
		_format(format),
		_bias(bias)
	{}

	virtual ~BrotherEncoder() {}

private:
	int _format;
	int _bias;
public:
    std::unique_ptr<Fluxmap> encode(int physicalTrack, int physicalSide, const SectorSet& allSectors);
};

extern FlagGroup brotherEncoderFlags;

#endif
