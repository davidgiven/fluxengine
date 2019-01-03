#ifndef BROTHER_H
#define BROTHER_H

/* Brother word processor format (or at least, one of them) */

#define BROTHER_SECTOR_RECORD 0xFFFFFD57
#define BROTHER_DATA_RECORD   0xFFFFFDDB
#define BROTHER_DATA_RECORD_PAYLOAD 256

class Sector;
class Fluxmap;

extern RecordVector decodeBitsToRecordsBrother(const std::vector<bool>& bitmap);

extern std::vector<std::unique_ptr<Sector>> parseRecordsToSectorsBrother(const RecordVector& records);

extern void writeBrotherSectorHeader(std::vector<bool>& bits, unsigned& cursor,
		int track, int sector);

#endif
