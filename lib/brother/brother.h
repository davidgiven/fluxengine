#ifndef BROTHER_H
#define BROTHER_H

/* Brother word processor format (or at least, one of them) */

#define BROTHER_SECTOR_RECORD 0xFFFFFD57
#define BROTHER_DATA_RECORD   0xFFFFFDDB
#define BROTHER_DATA_RECORD_PAYLOAD 256

class Sector;
class Fluxmap;

extern std::vector<std::vector<uint8_t>> decodeBitsToRecordsBrother(const std::vector<bool>& bitmap);

extern std::vector<std::unique_ptr<Sector>> parseRecordsToSectorsBrother(const std::vector<std::vector<uint8_t>>& records);

extern std::vector<bool> encodeRecordsToBits(const std::vector<std::vector<uint8_t>>& records);

extern std::vector<std::vector<std::unique_ptr<uint8_t>>> unparseSectorsToRecordsBrother(const std::vector<std::unique_ptr<Sector>>& sectors);

#endif
