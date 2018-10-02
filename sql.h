#ifndef SQL_H
#define SQL_H

#include <sqlite3.h>

extern void sql_check(sqlite3* db, int i);
extern sqlite3* sql_open(const char* filename);
extern void sql_stmt(sqlite3* db, const char* sql);
extern void sql_bind_blob(sqlite3* db, sqlite3_stmt* stmt, const char* name,
    const void* ptr, size_t bytes);
extern void sql_bind_int(sqlite3* db, sqlite3_stmt* stmt, const char* name, int value);

#endif
