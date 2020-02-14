#include "globals.h"
#include "fluxmap.h"
#include "sql.h"
#include "fluxsink/fluxsink.h"
#include "flags.h"
#include "fmt/format.h"
#include <unistd.h>

FlagGroup sqliteFluxSinkFlags;

static SettableFlag mergeFlag(
	{ "--merge" },
	"merge new data into existing flux file");

static SettableFlag overwriteFlag(
	{ "--overwrite" },
	"overwrite existing flux file");

class SqliteFluxSink : public FluxSink
{
public:
    SqliteFluxSink(const std::string& filename)
    {
		if (mergeFlag && overwriteFlag)
			Error() << "you can't specify --merge and --overwrite";

		if (!mergeFlag)
		{
			if (!overwriteFlag && (access(filename.c_str(), F_OK) == 0))
				Error() << "cowardly refusing to overwrite flux file without --merge or --overwrite specified";
			if ((access(filename.c_str(), F_OK) == 0) && (remove(filename.c_str()) != 0))
				Error() << fmt::format("failed to overwrite flux file");
		}
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


