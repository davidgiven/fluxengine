#ifndef TIDS990_H
#define TIDS990_H

#define TIDS990_RECORD_SIZE 0x516 /* bytes */
#define TIDS990_ID_SIZE 17
#define TIDS990_PAYLOAD_SIZE 0x500

class Sector;
class Fluxmap;
class Track;

class TiDs990Decoder : public AbstractDecoder
{
public:
    virtual ~TiDs990Decoder() {}

    RecordType advanceToNextRecord();
    void decodeSectorRecord();
};

#endif


