#ifndef AMIGA_H
#define AMIGA_H

#define BROTHER_SECTOR_RECORD 0xFFFFFD57
#define BROTHER_DATA_RECORD   0xFFFFFDDB
#define BROTHER_DATA_RECORD_PAYLOAD 256

class Sector;
class Fluxmap;

class AmigaBitmapDecoder : public BitmapDecoder
{
public:
	RecordVector decodeBitsToRecords(const std::vector<bool>& bitmap) const;
    nanoseconds_t guessClock(Fluxmap& fluxmap) const;
};

class AmigaRecordParser : public RecordParser
{
public:
	std::vector<std::unique_ptr<Sector>> parseRecordsToSectors(
		const RecordVector& records) const;
};


#endif
