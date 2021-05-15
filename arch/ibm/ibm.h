#ifndef IBM_H
#define IBM_H

#include "decoders/decoders.h"
#include "encoders/encoders.h"

class IbmDecoderProto;
class IbmEncoderProto;

/* IBM format (i.e. ordinary PC floppies). */

#define IBM_MFM_SYNC   0xA1   /* sync byte for MFM */
#define IBM_IAM        0xFC   /* start-of-track record */
#define IBM_IAM_LEN    1      /* plus prologue */
#define IBM_IDAM       0xFE   /* sector header */
#define IBM_IDAM_LEN   7      /* plus prologue */
#define IBM_DAM1       0xF8   /* sector data (type 1) */
#define IBM_DAM2       0xFB   /* sector data (type 2) */
#define IBM_TRS80DAM1  0xF9   /* sector data (TRS-80 directory) */
#define IBM_TRS80DAM2  0xFA   /* sector data (TRS-80 directory) */
#define IBM_DAM_LEN    1      /* plus prologue and user data */

/* Length of a DAM record is determined by the previous sector header. */

struct IbmIdam
{
    uint8_t id;
    uint8_t cylinder;
    uint8_t side;
    uint8_t sector;
    uint8_t sectorSize;
    uint8_t crc[2];
};

class IbmDecoder : public AbstractDecoder
{
public:
    IbmDecoder(const IbmDecoderProto& config):
		_config(config)
    {}

    RecordType advanceToNextRecord();
    void decodeSectorRecord();
    void decodeDataRecord();

	std::set<unsigned> requiredSectors(Track& track) const;

private:
	const IbmDecoderProto& _config;
    unsigned _currentSectorSize;
    unsigned _currentHeaderLength;
};

class IbmEncoder : public AbstractEncoder
{
public:
	IbmEncoder(const IbmEncoderProto& config):
		_config(config)
	{}

	virtual ~IbmEncoder() {}

public:
    std::unique_ptr<Fluxmap> encode(int physicalTrack, int physicalSide, const SectorSet& allSectors);

private:
	void writeRawBits(uint32_t data, int width);
	void writeBytes(const Bytes& bytes);
	void writeBytes(int count, uint8_t value);
	void writeSync();
	
private:
	const IbmEncoderProto& _config;
	std::vector<bool> _bits;
	unsigned _cursor;
	bool _lastBit;
};

#endif
