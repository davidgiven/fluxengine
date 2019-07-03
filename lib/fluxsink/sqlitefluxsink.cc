#include "globals.h"
#include "fluxmap.h"
#include "sql.h"
#include "fluxsink/fluxsink.h"
#include "fmt/format.h"

class SqliteFluxSink : public FluxSink
{
public:
    SqliteFluxSink(const std::string& filename)
    {
		_outdb = sqlOpen(filename, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
		int oldVersion = sqlReadIntProperty(_outdb, "version");
		if ((oldVersion != 0) && (oldVersion != FLUX_VERSION_CURRENT))
            Error() << fmt::format("that flux file is version {}, but this client is for version {}",
                oldVersion, FLUX_VERSION_CURRENT);

		sqlPrepareFlux(_outdb);
		sqlStmt(_outdb, "BEGIN;");
        sqlWriteIntProperty(_outdb, "version", FLUX_VERSION_CURRENT);
    }

    ~SqliteFluxSink()
    {
        if (_outdb)
		{
			sqlStmt(_outdb, "COMMIT;");
            sqlClose(_outdb);
		}
    }

public:
    void writeFlux(int track, int side, Fluxmap& fluxmap)
    {
        return sqlWriteFlux(_outdb, track, side, fluxmap);
    }

private:
    sqlite3* _outdb;
};

std::unique_ptr<FluxSink> FluxSink::createSqliteFluxSink(const std::string& filename)
{
    return std::unique_ptr<FluxSink>(new SqliteFluxSink(filename));
}


