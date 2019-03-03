#include "globals.h"
#include "flags.h"
#include "sql.h"
#include "fmt/format.h"

static sqlite3* db;

int main(int argc, const char* argv[])
{
    if (argc != 2)
        Error() << "syntax: fe-upgradefluxfile <fluxfile>";
    std::string filename = argv[1];
    
    db = sqlOpen(filename, SQLITE_OPEN_READWRITE);
    atexit([]()
        {
            sqlClose(db);
        }
    );

    int version = sqlGetVersion(db);
    std::cout << fmt::format("File at version {}\n", version);
    if (version == FLUX_VERSION_CURRENT)
    {
        std::cout << "Up to date!\n";
        return 0;
    }

    if (version == FLUX_VERSION_0)
    {
        std::cout << "Updating to version 1\n";
        sqlPrepareFlux(db);
        sqlStmt(db, "BEGIN;");
        sqlStmt(db,
            "INSERT INTO zdata"
            " SELECT track, side, data, 0 AS compression FROM rawdata;"
        );
        sqlStmt(db, "DROP TABLE rawdata;");
        sqlWriteIntProperty(db, "version", FLUX_VERSION_1);
        sqlStmt(db, "COMMIT;");
        version = FLUX_VERSION_1;
    }

    std::cout << "Upgrade done\n";
    return 0;
}
