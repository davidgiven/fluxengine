#include "globals.h"
#include "sql.h"

void sql_check(sqlite3* db, int i)
{
    if (i != SQLITE_OK)
        error("database error: %s", sqlite3_errmsg(db));
}

sqlite3* sql_open(const char* filename)
{
    sqlite3* db;
    int i = sqlite3_open_v2(filename, &db,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
        NULL);
    if (i != SQLITE_OK)
        error("failed to open output file: %s", sqlite3_errstr(i));

    return db;
}

void sql_stmt(sqlite3* db, const char* sql)
{
    char* errmsg;
    int i = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
    if (i != SQLITE_OK)
        error("database error: %s", errmsg);
}

void sql_bind_blob(sqlite3* db, sqlite3_stmt* stmt, const char* name,
    const void* ptr, size_t bytes)
{
    sql_check(db, sqlite3_bind_blob(stmt,
        sqlite3_bind_parameter_index(stmt, name),
        ptr, bytes, SQLITE_TRANSIENT));
}

void sql_bind_int(sqlite3* db, sqlite3_stmt* stmt, const char* name, int value)
{
    sql_check(db, sqlite3_bind_int(stmt,
        sqlite3_bind_parameter_index(stmt, name),
        value));
}
