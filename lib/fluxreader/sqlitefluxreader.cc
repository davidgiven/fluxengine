#include "globals.h"
#include "fluxmap.h"
#include "sql.h"
#include "fluxreader.h"
#include "fmt/format.h"

class SqliteFluxReader : public FluxReader
{
public:
    SqliteFluxReader(const std::string& filename)
    {
        _indb = sqlOpen(filename, SQLITE_OPEN_READONLY);
        int version = sqlGetVersion(_indb);
        if (version != FLUX_VERSION_CURRENT)
            Error() << fmt::format("that flux file is version {}, but this client is for version {}",
                version, FLUX_VERSION_CURRENT);
    }

    ~SqliteFluxReader()
    {
        if (_indb)
            sqlClose(_indb);
    }

public:
    std::unique_ptr<Fluxmap> readFlux(int track, int side)
    {
        return sqlReadFlux(_indb, track, side);
    }

    void recalibrate() {}

private:
    sqlite3* _indb;
};

std::unique_ptr<FluxReader> FluxReader::createSqliteFluxReader(const std::string& filename)
{
    return std::unique_ptr<FluxReader>(new SqliteFluxReader(filename));
}


