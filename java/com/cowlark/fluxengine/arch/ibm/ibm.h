#ifndef IBM_H
#define IBM_H

/* IBM format (i.e. ordinary PC floppies). */

#define IBM_MFM_SYNC 0xA1  /* sync byte for MFM */
#define IBM_IAM 0xFC       /* start-of-track record */
#define IBM_IAM_LEN 1      /* plus prologue */
#define IBM_IDAM 0xFE      /* sector header */
#define IBM_IDAM_LEN 7     /* plus prologue */
#define IBM_DAM1 0xF8      /* sector data (type 1) */
#define IBM_DAM2 0xFB      /* sector data (type 2) */
#define IBM_TRS80DAM1 0xF9 /* sector data (TRS-80 directory) */
#define IBM_TRS80DAM2 0xFA /* sector data (TRS-80 directory) */
#define IBM_DAM_LEN 1      /* plus prologue and user data */

/* Length of a DAM record is determined by the previous sector header. */

struct IbmIdam
{
    uint8_t id;
    uint8_t track;
    uint8_t side;
    uint8_t sector;
    uint8_t sectorSize;
    uint8_t crc[2];
};

class Encoder;
class Decoder;
class DecoderProto;
class EncoderProto;

extern std::unique_ptr<Decoder> createIbmDecoder(const DecoderProto& config);
extern std::unique_ptr<Encoder> createIbmEncoder(const EncoderProto& config);

#endif
