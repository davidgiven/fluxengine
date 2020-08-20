#ifndef TIDS990_H
#define TIDS990_H

#define TIDS990_PAYLOAD_SIZE       288 /* bytes */
#define TIDS990_SECTOR_RECORD_SIZE 10 /* bytes */
#define TIDS990_DATA_RECORD_SIZE   (TIDS990_PAYLOAD_SIZE + 4) /* bytes */

class Sector;
class Fluxmap;
class Track;

class TiDs990Decoder : public AbstractDecoder
{
public:
    virtual ~TiDs990Decoder() {}

    RecordType advanceToNextRecord();
    void decodeSectorRecord();
	void decodeDataRecord();
};

#endif


