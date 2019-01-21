#ifndef BROTHER_H
#define BROTHER_H

/* Brother word processor format (or at least, one of them) */

#define BROTHER_SECTOR_RECORD 0xFFFFFD57
#define BROTHER_DATA_RECORD   0xFFFFFDDB
#define BROTHER_DATA_RECORD_PAYLOAD 256

class Sector;
class Fluxmap;

extern RecordVector decodeBitsToRecordsBrother(const std::vector<bool>& bitmap);

class BrotherRecordParser : public RecordParser
{
public:

	std::vector<std::unique_ptr<Sector>> parseRecordsToSectors(
		const RecordVector& records);
};

extern void writeBrotherSectorHeader(std::vector<bool>& bits, unsigned& cursor,
		int track, int sector);
extern void writeBrotherSectorData(std::vector<bool>& bits, unsigned& cursor,
		const std::vector<uint8_t>& data);

#endif
