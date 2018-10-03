#ifndef SQL_H
#define SQL_H

#include <sqlite3.h>

extern void sql_check(sqlite3* db, int i);
extern sqlite3* sql_open(const char* filename, int flags);
extern void sql_stmt(sqlite3* db, const char* sql);
extern void sql_bind_blob(sqlite3* db, sqlite3_stmt* stmt, const char* name,
    const void* ptr, size_t bytes);
extern void sql_bind_int(sqlite3* db, sqlite3_stmt* stmt, const char* name, int value);

extern void sql_prepare_raw(sqlite3* db);
extern void sql_write_raw(sqlite3* db, int track, int side, const void* ptr, size_t len);
extern void sql_for_all_raw_data(sqlite3* db, void (*cb)(int track, int side, const uint8_t* ptr, size_t len));
extern void sql_read_raw(sqlite3* db, int track, int side, void** ptr, size_t* len);

#endif
