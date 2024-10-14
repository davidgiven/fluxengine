#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/fluxsink/fluxsink.h"
#include "lib/core/bytes.h"
#include "fmt/format.h"
#include <string.h>
#include <fstream>
#include <sqlite3.h>

/* --- SQL library ------------------------------------------------------- */

enum
{
    FLUX_VERSION_0, /* without properties table */
    FLUX_VERSION_1,
    FLUX_VERSION_2, /* new bytecode with index marks */
    FLUX_VERSION_3, /* simplified bytecode with six-bit timer */
};

enum
{
    COMPRESSION_NONE,
    COMPRESSION_ZLIB
};

static bool hasProperties(sqlite3* db);

void sqlCheck(sqlite3* db, int i)
{
    if (i != SQLITE_OK)
        error("database error: {}", sqlite3_errmsg(db));
}

sqlite3* sqlOpen(const std::string filename, int flags)
{
    sqlite3* db;
    int i = sqlite3_open_v2(filename.c_str(), &db, flags, NULL);
    if (i != SQLITE_OK)
        error("failed: {}", sqlite3_errstr(i));

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
        error("database error: {}", errmsg);
}

bool hasProperties(sqlite3* db)
{
    sqlite3_stmt* stmt;
    sqlCheck(db,
        sqlite3_prepare_v2(db,
            "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND "
            "name='properties'",
            -1,
            &stmt,
            NULL));

    int i = sqlite3_step(stmt);
    if (i != SQLITE_ROW)
        error("Error accessing sqlite metadata");
    bool has_properties = sqlite3_column_int(stmt, 0) != 0;
    sqlCheck(db, sqlite3_finalize(stmt));

    return has_properties;
}

void sql_bind_blob(sqlite3* db,
    sqlite3_stmt* stmt,
    const char* name,
    const void* ptr,
    size_t bytes)
{
    sqlCheck(db,
        sqlite3_bind_blob(stmt,
            sqlite3_bind_parameter_index(stmt, name),
            ptr,
            bytes,
            SQLITE_TRANSIENT));
}

void sql_bind_int(sqlite3* db, sqlite3_stmt* stmt, const char* name, int value)
{
    sqlCheck(db,
        sqlite3_bind_int(
            stmt, sqlite3_bind_parameter_index(stmt, name), value));
}

void sql_bind_string(
    sqlite3* db, sqlite3_stmt* stmt, const char* name, const char* value)
{
    sqlCheck(db,
        sqlite3_bind_text(stmt,
            sqlite3_bind_parameter_index(stmt, name),
            value,
            -1,
            SQLITE_TRANSIENT));
}

std::vector<std::pair<unsigned, unsigned>> sqlFindFlux(sqlite3* db)
{
    std::vector<std::pair<unsigned, unsigned>> output;

    sqlite3_stmt* stmt;
    sqlCheck(db,
        sqlite3_prepare_v2(
            db, "SELECT track, side FROM zdata", -1, &stmt, NULL));

    for (;;)
    {
        int i = sqlite3_step(stmt);
        if (i == SQLITE_DONE)
            break;
        if (i != SQLITE_ROW)
            error("failed to read from database: {}", sqlite3_errmsg(db));

        unsigned track = sqlite3_column_int(stmt, 0);
        unsigned side = sqlite3_column_int(stmt, 1);
        output.push_back(std::make_pair(track, side));
    }
    sqlCheck(db, sqlite3_finalize(stmt));

    return output;
}

long sqlReadIntProperty(sqlite3* db, const std::string& name)
{
    if (!hasProperties(db))
        return 0;

    sqlite3_stmt* stmt;
    sqlCheck(db,
        sqlite3_prepare_v2(db,
            "SELECT value FROM properties WHERE key=:key",
            -1,
            &stmt,
            NULL));
    sql_bind_string(db, stmt, ":key", name.c_str());

    int i = sqlite3_step(stmt);
    long result = 0;
    if (i != SQLITE_DONE)
    {
        if (i != SQLITE_ROW)
            error("failed to read from database: {}", sqlite3_errmsg(db));

        result = sqlite3_column_int(stmt, 0);
    }
    sqlCheck(db, sqlite3_finalize(stmt));
    return result;
}

int sqlGetVersion(sqlite3* db)
{
    return sqlReadIntProperty(db, "version");
}

/* --- Actual program ---------------------------------------------------- */

static void syntax()
{
    std::cerr
        << "syntax: upgrade-flux-file <filename>\n"
        << "This tool upgrades the flux file in-place to the current format.\n";
    exit(0);
}

static Bytes sqlReadFluxBytes(sqlite3* db, int track, int side)
{
    sqlite3_stmt* stmt;
    sqlCheck(db,
        sqlite3_prepare_v2(db,
            "SELECT data, compression FROM zdata WHERE track=:track AND "
            "side=:side",
            -1,
            &stmt,
            NULL));
    sql_bind_int(db, stmt, ":track", track);
    sql_bind_int(db, stmt, ":side", side);

    Bytes data;
    int i = sqlite3_step(stmt);
    if (i != SQLITE_DONE)
    {
        if (i != SQLITE_ROW)
            error("failed to read from database: {}", sqlite3_errmsg(db));

        const uint8_t* blobptr = (const uint8_t*)sqlite3_column_blob(stmt, 0);
        size_t bloblen = sqlite3_column_bytes(stmt, 0);
        int compression = sqlite3_column_int(stmt, 1);
        data = Bytes(blobptr, bloblen);

        switch (compression)
        {
            case COMPRESSION_NONE:
                break;

            case COMPRESSION_ZLIB:
                data = data.decompress();
                break;

            default:
                error("unsupported compression type {}", compression);
        }
    }
    sqlCheck(db, sqlite3_finalize(stmt));
    return data;
}

static bool isSqlite(const std::string& filename)
{
    char buffer[16];
    std::ifstream(filename, std::ios::in | std::ios::binary).read(buffer, 16);
    if (strncmp(buffer, "SQLite format 3", 16) == 0)
        return true;
    return false;
}

static void translateFluxVersion2(Fluxmap& fluxmap, const Bytes& bytes)
{
    unsigned pending = 0;
    for (uint8_t b : bytes)
    {
        switch (b)
        {
            case 0x80: /* pulse */
                fluxmap.appendInterval(pending);
                fluxmap.appendPulse();
                pending = 0;
                break;

            case 0x81: /* index */
                fluxmap.appendInterval(pending);
                fluxmap.appendIndex();
                pending = 0;
                break;

            default:
                pending += b;
                break;
        }
    }
    fluxmap.appendInterval(pending);
}

int main(int argc, const char* argv[])
{
    try
    {
        if ((argc != 2) || (strcmp(argv[1], "--help") == 0))
            syntax();

        std::string filename = argv[1];
        if (!isSqlite(filename))
        {
            std::cout << "File is up to date.\n";
            exit(0);
        }

        std::string outFilename = filename + ".out.flux";
        auto db = sqlOpen(filename, SQLITE_OPEN_READONLY);
        int version = sqlGetVersion(db);

        {
            auto fluxsink = FluxSink::createFl2FluxSink(outFilename);
            for (const auto& locations : sqlFindFlux(db))
            {
                unsigned cylinder = locations.first;
                unsigned head = locations.second;
                Bytes bytes = sqlReadFluxBytes(db, cylinder, head);
                Fluxmap fluxmap;
                switch (version)
                {
                    case FLUX_VERSION_2:
                        translateFluxVersion2(fluxmap, bytes);
                        break;

                    case FLUX_VERSION_3:
                        fluxmap.appendBytes(bytes);
                        break;

                    default:
                        error(
                            "you cannot upgrade version {} files (please file "
                            "a "
                            "bug)",
                            version);
                }
                fluxsink->writeFlux(cylinder, head, fluxmap);
                std::cout << '.' << std::flush;
            }

            std::cout << "Writing output file...\n";
        }

        sqlite3_close(db);

        if (remove(filename.c_str()) != 0)
            error("couldn't remove input file: {}", strerror(errno));

        if (rename(outFilename.c_str(), filename.c_str()) != 0)
            error("couldn't replace input file: {}", strerror(errno));
        return 0;
    }
    catch (const ErrorException& e)
    {
        e.print();
        exit(1);
    }
}
