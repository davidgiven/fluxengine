#ifndef VICTOR9K_H
#define VICTOR9K_H

#define VICTOR9K_SECTOR_RECORD 0xfffffeab
#define VICTOR9K_DATA_RECORD   0xfffffea4

#define VICTOR9K_SECTOR_LENGTH 512

class Sector;
class Fluxmap;
class Victor9kDecoderProto;

class Victor9kDecoder : public AbstractDecoder
{
public:
	Victor9kDecoder(const Victor9kDecoderProto&) {}
    virtual ~Victor9kDecoder() {}

    RecordType advanceToNextRecord();
    void decodeSectorRecord();
    void decodeDataRecord();
};

#endif
