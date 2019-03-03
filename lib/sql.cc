#include "globals.h"
#include "sql.h"
#include "fluxmap.h"

void sqlCheck(sqlite3* db, int i)
{
    if (i != SQLITE_OK)
        Error() << "database error: " << sqlite3_errmsg(db);
}

sqlite3* sqlOpen(const std::string filename, int flags)
{
    sqlite3* db;
    int i = sqlite3_open_v2(filename.c_str(), &db, flags, NULL);
    if (i != SQLITE_OK)
        Error() << "failed to open output file: " << sqlite3_errstr(i);

    return db;
}

void sqlClose(sqlite3* db)
{
	sqlite3_close(db);
}

void sqlStmt(sqlite3* db, const char* sql)
{
    char* errmsg;
    int i = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
    if (i != SQLITE_OK)
        Error() << "database error: %s" << errmsg;
}

void sql_bind_blob(sqlite3* db, sqlite3_stmt* stmt, const char* name,
    const void* ptr, size_t bytes)
{
    sqlCheck(db, sqlite3_bind_blob(stmt,
        sqlite3_bind_parameter_index(stmt, name),
        ptr, bytes, SQLITE_TRANSIENT));
}

void sql_bind_int(sqlite3* db, sqlite3_stmt* stmt, const char* name, int value)
{
    sqlCheck(db, sqlite3_bind_int(stmt,
        sqlite3_bind_parameter_index(stmt, name),
        value));
}

void sql_bind_string(sqlite3* db, sqlite3_stmt* stmt, const char* name, const char* value)
{
    sqlCheck(db, sqlite3_bind_text(stmt,
        sqlite3_bind_parameter_index(stmt, name),
        value, -1, SQLITE_TRANSIENT));
}

void sqlPrepareFlux(sqlite3* db)
{
    sqlStmt(db, "PRAGMA synchronous = OFF;");
    sqlStmt(db, "CREATE TABLE IF NOT EXISTS properties ("
                 "  key TEXT UNIQUE NOT NULL PRIMARY KEY,"
                 "  value TEXT"
                 ");");
    sqlStmt(db, "CREATE TABLE IF NOT EXISTS rawdata ("
                 "  track INTEGER,"
                 "  side INTEGER,"
                 "  data BLOB,"
                 "  PRIMARY KEY(track, side)"
                 ");");
}

void sqlWriteFlux(sqlite3* db, int track, int side, const Fluxmap& fluxmap)
{
    sqlite3_stmt* stmt;
    sqlCheck(db, sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO rawdata (track, side, data) VALUES (:track, :side, :data)",
        -1, &stmt, NULL));
    sql_bind_int(db, stmt, ":track", track);
    sql_bind_int(db, stmt, ":side", side);
    sql_bind_blob(db, stmt, ":data", fluxmap.ptr(), fluxmap.bytes());

    if (sqlite3_step(stmt) != SQLITE_DONE)
        Error() << "failed to write to database: " << sqlite3_errmsg(db);
    sqlCheck(db, sqlite3_finalize(stmt));
}

std::unique_ptr<Fluxmap> sqlReadFlux(sqlite3* db, int track, int side)
{
    sqlite3_stmt* stmt;
    sqlCheck(db, sqlite3_prepare_v2(db,
        "SELECT data FROM rawdata WHERE track=:track AND side=:side",
        -1, &stmt, NULL));
    sql_bind_int(db, stmt, ":track", track);
    sql_bind_int(db, stmt, ":side", side);

    auto fluxmap = std::unique_ptr<Fluxmap>(new Fluxmap());

    int i = sqlite3_step(stmt);
    if (i != SQLITE_DONE)
    {
        if (i != SQLITE_ROW)
            Error() << "failed to read from database: " << sqlite3_errmsg(db);

        const uint8_t* blobptr = (const uint8_t*) sqlite3_column_blob(stmt, 0);
        size_t bloblen = sqlite3_column_bytes(stmt, 0);

        fluxmap->appendBytes(blobptr, bloblen);
    }
    sqlCheck(db, sqlite3_finalize(stmt));
    return fluxmap;
}

void sqlWriteStringProperty(sqlite3* db, const std::string& name, const std::string& value)
{
    sqlite3_stmt* stmt;
    sqlCheck(db, sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO properties (key, value) VALUES (:key, :value)",
        -1, &stmt, NULL));
    sql_bind_string(db, stmt, ":key", name.c_str());
    sql_bind_string(db, stmt, ":value", value.c_str());

    if (sqlite3_step(stmt) != SQLITE_DONE)
        Error() << "failed to write to database: " << sqlite3_errmsg(db);
    sqlCheck(db, sqlite3_finalize(stmt));
}

std::string sqlReadStringProperty(sqlite3* db, const std::string& name)
{
    sqlite3_stmt* stmt;
    sqlCheck(db, sqlite3_prepare_v2(db,
        "SELECT value FROM properties WHERE key=:key",
        -1, &stmt, NULL));
    sql_bind_string(db, stmt, ":key", name.c_str());

    int i = sqlite3_step(stmt);
    std::string result;
    if (i != SQLITE_DONE)
    {
        if (i != SQLITE_ROW)
            Error() << "failed to read from database: " << sqlite3_errmsg(db);

        result = (const char*) sqlite3_column_text(stmt, 0);
    }
    sqlCheck(db, sqlite3_finalize(stmt));
    return result;
}