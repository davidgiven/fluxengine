#include "globals.h"
#include "sql.h"
#include "fluxmap.h"
#include "bytes.h"
#include "fmt/format.h"

enum
{
    COMPRESSION_NONE,
    COMPRESSION_ZLIB
};

static bool hasProperties(sqlite3* db);

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
        Error() << "failed: " << sqlite3_errstr(i);

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

int sqlGetVersion(sqlite3* db)
{
    return sqlReadIntProperty(db, "version");
}

bool hasProperties(sqlite3* db)
{
    sqlite3_stmt* stmt;
    sqlCheck(db, sqlite3_prepare_v2(db,
        "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='properties'",
        -1, &stmt, NULL));

    int i = sqlite3_step(stmt);
    if (i != SQLITE_ROW)
        Error() << "Error accessing sqlite metadata";
    bool has_properties = sqlite3_column_int(stmt, 0) != 0;
    sqlCheck(db, sqlite3_finalize(stmt));

    return has_properties;
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
    sqlStmt(db, "BEGIN;");
    sqlStmt(db, "CREATE TABLE IF NOT EXISTS properties ("
                 "  key TEXT UNIQUE NOT NULL PRIMARY KEY,"
                 "  value TEXT"
                 ");");
    sqlStmt(db, "CREATE TABLE IF NOT EXISTS zdata ("
                 "  track INTEGER,"
                 "  side INTEGER,"
                 "  data BLOB,"
                 "  compression INTEGER,"
                 "  PRIMARY KEY(track, side)"
                 ");");
    sqlStmt(db, "COMMIT;");
}

void sqlWriteFlux(sqlite3* db, int track, int side, const Fluxmap& fluxmap)
{
    const auto compressed = compress(fluxmap.rawBytes());

    sqlite3_stmt* stmt;
    sqlCheck(db, sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO zdata (track, side, data, compression)"
            " VALUES (:track, :side, :data, :compression)",
        -1, &stmt, NULL));
    sql_bind_int(db, stmt, ":track", track);
    sql_bind_int(db, stmt, ":side", side);
    sql_bind_blob(db, stmt, ":data", &compressed[0], compressed.size());
    sql_bind_int(db, stmt, ":compression", COMPRESSION_ZLIB);

    if (sqlite3_step(stmt) != SQLITE_DONE)
        Error() << "failed to write to database: " << sqlite3_errmsg(db);
    sqlCheck(db, sqlite3_finalize(stmt));
}

std::unique_ptr<Fluxmap> sqlReadFlux(sqlite3* db, int track, int side)
{
    sqlite3_stmt* stmt;
    sqlCheck(db, sqlite3_prepare_v2(db,
        "SELECT data, compression FROM zdata WHERE track=:track AND side=:side",
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
        int compression = sqlite3_column_int(stmt, 1);
        std::vector<uint8_t> data(blobptr, blobptr+bloblen);

        switch (compression)
        {
            case COMPRESSION_NONE:
                break;

            case COMPRESSION_ZLIB:
                data = decompress(data);
                break;

            default:
                Error() << fmt::format("unsupported compression type {}", compression);
        }

        fluxmap->appendBytes(data);
    }
    sqlCheck(db, sqlite3_finalize(stmt));
    return fluxmap;
}

void sqlWriteStringProperty(sqlite3* db, const std::string& name, const std::string& value)
{
    if (!hasProperties(db))
        sqlPrepareFlux(db);

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
    if (!hasProperties(db))
        return "";

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

void sqlWriteIntProperty(sqlite3* db, const std::string& name, long value)
{
    if (!hasProperties(db))
        sqlPrepareFlux(db);

    sqlite3_stmt* stmt;
    sqlCheck(db, sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO properties (key, value) VALUES (:key, :value)",
        -1, &stmt, NULL));
    sql_bind_string(db, stmt, ":key", name.c_str());
    sql_bind_int(db, stmt, ":value", value);

    if (sqlite3_step(stmt) != SQLITE_DONE)
        Error() << "failed to write to database: " << sqlite3_errmsg(db);
    sqlCheck(db, sqlite3_finalize(stmt));
}

long sqlReadIntProperty(sqlite3* db, const std::string& name)
{
    if (!hasProperties(db))
        return 0;

    sqlite3_stmt* stmt;
    sqlCheck(db, sqlite3_prepare_v2(db,
        "SELECT value FROM properties WHERE key=:key",
        -1, &stmt, NULL));
    sql_bind_string(db, stmt, ":key", name.c_str());

    int i = sqlite3_step(stmt);
    long result = 0;
    if (i != SQLITE_DONE)
    {
        if (i != SQLITE_ROW)
            Error() << "failed to read from database: " << sqlite3_errmsg(db);

        result = sqlite3_column_int(stmt, 0);
    }
    sqlCheck(db, sqlite3_finalize(stmt));
    return result;
}