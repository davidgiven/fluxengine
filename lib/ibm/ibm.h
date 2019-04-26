#ifndef IBM_H
#define IBM_H

#include "decoders.h"

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

class IbmDecoder : public AbstractSplitDecoder
{
public:
    IbmDecoder(unsigned sectorBase):
        _sectorBase(sectorBase)
    {}

    nanoseconds_t findSector(FluxmapReader& fmr, Track& track) override;
    nanoseconds_t findData(FluxmapReader& fmr, Track& track) override;
    void decodeHeader(FluxmapReader& fmr, Track& track, Sector& sector) override;
    void decodeData(FluxmapReader& fmr, Track& track, Sector& sector) override;

private:
    unsigned _sectorBase;
    unsigned _sectorSize;
};

#if 0
class AbstractIbmDecoder : public AbstractSoftSectorDecoder
{
public:
    AbstractIbmDecoder(unsigned sectorIdBase):
        _sectorIdBase(sectorIdBase)
    {}
    virtual ~AbstractIbmDecoder() {}

    SectorVector decodeToSectors(const RawRecordVector& rawRecords, unsigned physicalTrack, unsigned physicalSide);

protected:
    virtual int skipHeaderBytes() const = 0;

private:
    unsigned _sectorIdBase;
};

class IbmFmDecoder : public AbstractIbmDecoder
{
public:
    IbmFmDecoder(unsigned sectorIdBase):
        AbstractIbmDecoder(sectorIdBase)
    {}

    int recordMatcher(uint64_t fifo) const;

protected:
    int skipHeaderBytes() const
    { return 0; }
};

class IbmMfmDecoder : public AbstractIbmDecoder
{
public:
    IbmMfmDecoder(unsigned sectorIdBase):
        AbstractIbmDecoder(sectorIdBase)
    {}

    nanoseconds_t guessClock(Fluxmap& fluxmap) const;
    int recordMatcher(uint64_t fifo) const;

protected:
    int skipHeaderBytes() const
    { return 3; }
};
#endif

#endif
