#ifndef IBM_H
#define IBM_H

#include "decoders/decoders.h"
#include "encoders/encoders.h"

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
    IbmDecoder(unsigned sectorBase, bool ignoreSideByte=false):
        _sectorBase(sectorBase),
        _ignoreSideByte(ignoreSideByte)
    {}

    RecordType advanceToNextRecord();
    void decodeSectorRecord();
    void decodeDataRecord();

private:
    unsigned _sectorBase;
    bool _ignoreSideByte;
    unsigned _currentSectorSize;
    unsigned _currentHeaderLength;
};

struct IbmParameters
{
	int sectorsPerTrack;
	int sectorSize;
	bool emitIam;
	int startSectorId;
	int clockSpeedKhz;
	bool useFm;
	int gap1;
	int gap3;
	uint8_t damByte;
	std::string sectorSkew;
};

class IbmEncoder : public AbstractEncoder
{
public:
	IbmEncoder(const IbmParameters& parameters):
		parameters(parameters)
	{}

	virtual ~IbmEncoder() {}

public:
    std::unique_ptr<Fluxmap> encode(int physicalTrack, int physicalSide, const SectorSet& allSectors);

private:
	IbmParameters parameters;
};

#endif
