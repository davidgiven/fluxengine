#ifndef SQL_H
#define SQL_H

#include <sqlite3.h>

class Fluxmap;

extern void sqlCheck(sqlite3* db, int i);
extern sqlite3* sqlOpen(const std::string filename, int flags);
extern void sqlStmt(sqlite3* db, const char* sql);

extern void sqlPrepareFlux(sqlite3* db);
extern void sqlWriteFlux(sqlite3* db, int track, int side, const struct fluxmap* fluxmap);
extern std::unique_ptr<Fluxmap> sqlReadFlux(sqlite3* db, int track, int side);

#if 0
extern void sqlfor_all_flux_data(sqlite3* db, void (*cb)(int track, int side, const struct fluxmap* fluxmap));

extern void sqlprepare_record(sqlite3* db);
extern void sqlwrite_record(sqlite3* db, int track, int side, int record, const uint8_t* ptr, size_t len);
extern void sqlfor_all_record_data(sqlite3* db, void (*cb)(int track, int side, int record, const uint8_t* ptr, size_t len));
#endif

#endif
