#ifndef SQL_H
#define SQL_H

#include <sqlite3.h>

extern void sql_check(sqlite3* db, int i);
extern sqlite3* sql_open(const char* filename, int flags);
extern void sql_stmt(sqlite3* db, const char* sql);
extern void sql_bind_blob(sqlite3* db, sqlite3_stmt* stmt, const char* name,
    const void* ptr, size_t bytes);
extern void sql_bind_int(sqlite3* db, sqlite3_stmt* stmt, const char* name, int value);

extern void sql_prepare_flux(sqlite3* db);
extern void sql_write_flux(sqlite3* db, int track, int side, const struct fluxmap* fluxmap);
extern struct fluxmap* sql_read_flux(sqlite3* db, int track, int side);
extern void sql_for_all_flux_data(sqlite3* db, void (*cb)(int track, int side, const struct fluxmap* fluxmap));

extern void sql_prepare_record(sqlite3* db);
extern void sql_write_record(sqlite3* db, int track, int side, int record, const uint8_t* ptr, size_t len);
extern void sql_for_all_record_data(sqlite3* db, void (*cb)(int track, int side, int record, const uint8_t* ptr, size_t len));

#endif
