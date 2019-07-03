#include "globals.h"
#include "fluxmap.h"
#include "sql.h"
#include "fluxsource/fluxsource.h"
#include "fmt/format.h"

class SqliteFluxSource : public FluxSource
{
public:
    SqliteFluxSource(const std::string& filename)
    {
        _indb = sqlOpen(filename, SQLITE_OPEN_READONLY);
        int version = sqlGetVersion(_indb);
        if (version != FLUX_VERSION_CURRENT)
            Error() << fmt::format("that flux file is version {}, but this client is for version {}",
                version, FLUX_VERSION_CURRENT);
    }

    ~SqliteFluxSource()
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

std::unique_ptr<FluxSource> FluxSource::createSqliteFluxSource(const std::string& filename)
{
    return std::unique_ptr<FluxSource>(new SqliteFluxSource(filename));
}


