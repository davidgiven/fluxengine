#include "globals.h"
#include "flags.h"
#include "sql.h"
#include "fluxmap.h"
#include "fmt/format.h"

static sqlite3* db;

static void update_version_1_to_2()
{
    for (const auto i : sqlFindFlux(db))
    {
        Fluxmap after;
        const auto before = sqlReadFlux(db, i.first, i.second);

        /* Remember, before does not contain valid opcodes! */
        unsigned pending = 0;
        for (uint8_t b : before->rawBytes())
        {
            if (b < 0x80)
            {
                after.appendInterval(b + pending);
                after.appendPulse();
                pending = 0;
            }
            else
                pending += 0x80;
        }

        sqlWriteFlux(db, i.first, i.second, after);
        std::cout << '.' << std::flush;
    }
    std::cout << std::endl;
}

int mainUpgradeFluxFile(int argc, const char* argv[])
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
        std::cout << "Upgrading to version 1\n";
        sqlPrepareFlux(db);
        sqlStmt(db, "BEGIN;");
        sqlStmt(db,
            "INSERT INTO zdata"
            " SELECT track, side, data, 0 AS compression FROM rawdata;"
        );
        sqlStmt(db, "DROP TABLE rawdata;");
        version = FLUX_VERSION_1;
        sqlWriteIntProperty(db, "version", version);
        sqlStmt(db, "COMMIT;");
    }

    if (version == FLUX_VERSION_1)
    {
        std::cout << "Upgrading to version 2\n";
        sqlStmt(db, "BEGIN;");
        update_version_1_to_2();
        version = FLUX_VERSION_2;
        sqlWriteIntProperty(db, "version", version);
        sqlStmt(db, "COMMIT;");
    }

    std::cout << "Vacuuming\n";
    sqlStmt(db, "VACUUM;");
    std::cout << "Upgrade done\n";
    return 0;
}
