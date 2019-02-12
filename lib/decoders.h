#ifndef DECODERS_H
#define DECODERS_H

/* IBM record scheme variants */

enum
{
    IBM_SCHEME_MFM,
    IBM_SCHEME_FM
};

/* IBM format (i.e. ordinary PC floppies). */

#define IBM_IAM        0xFC   /* start-of-track record */
#define IBM_IAM_LEN    1      /* plus prologue */
#define IBM_IDAM       0xFE   /* sector header */
#define IBM_IDAM_LEN   7      /* plus prologue */
#define IBM_DAM1       0xF8   /* sector data (type 1) */
#define IBM_DAM2       0xFB   /* sector data (type 2) */
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

class Sector;
class Fluxmap;
class Record;
typedef std::vector<std::unique_ptr<Record>> RecordVector;

class BitmapDecoder
{
public:
    virtual ~BitmapDecoder() {}

    virtual nanoseconds_t guessClock(Fluxmap& fluxmap) const;

    virtual RecordVector decodeBitsToRecords(
        const std::vector<bool>& bitmap) const = 0;
};

class FmBitmapDecoder : public BitmapDecoder
{
public:
    nanoseconds_t guessClock(Fluxmap& fluxmap) const;
    RecordVector decodeBitsToRecords(const std::vector<bool>& bitmap) const;
};

class MfmBitmapDecoder : public BitmapDecoder
{
public:
    nanoseconds_t guessClock(Fluxmap& fluxmap) const;
    RecordVector decodeBitsToRecords(const std::vector<bool>& bitmap) const;
};

class RecordParser
{
public:
    virtual ~RecordParser() {}

    virtual std::vector<std::unique_ptr<Sector>> parseRecordsToSectors(
        const RecordVector& records) const = 0;
};

class IbmRecordParser : public RecordParser
{
public:
    IbmRecordParser(int scheme, int sectorIdBase):
        _scheme(scheme),
        _sectorIdBase(sectorIdBase)
    {}

    std::vector<std::unique_ptr<Sector>> parseRecordsToSectors(
        const RecordVector& records) const;

private:
    int _scheme;
    int _sectorIdBase;
};

#endif
