#ifndef DECODERS_H
#define DECODERS_H

/* IBM format (i.e. ordinary PC floppies). */

#define IBM_IAM        0xFC   /* start-of-track record */
#define IBM_IAM_LEN    4
#define IBM_IDAM       0xFE  /* sector header */
#define IBM_IDAM_LEN   10
#define IBM_DAM1       0xF8  /* sector data (type 1) */
#define IBM_DAM2       0xFB  /* sector data (type 2) */
#define IBM_DAM_LEN    6 /* plus user data */
/* Length of a DAM record is determined by the previous sector header. */

struct IbmIdam
{
    uint8_t marker[3];
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

extern RecordVector decodeBitsToRecordsMfm(const std::vector<bool>& bitmap);

class RecordParser
{
public:
    virtual ~RecordParser() {}

    virtual std::vector<std::unique_ptr<Sector>> parseRecordsToSectors(
        const RecordVector& records) = 0;
};

class IbmRecordParser : public RecordParser
{
public:
    IbmRecordParser(int sectorIdBase):
        _sectorIdBase(sectorIdBase)
    {}

    std::vector<std::unique_ptr<Sector>> parseRecordsToSectors(
        const RecordVector& records);

private:
    int _sectorIdBase;
};

#endif
