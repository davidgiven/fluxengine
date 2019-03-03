#ifndef SQL_H
#define SQL_H

#include <sqlite3.h>

class Fluxmap;

enum
{
    FLUX_VERSION_0, /* without properties table */
    FLUX_VERSION_1,

    FLUX_VERSION_CURRENT = 1,
};

extern void sqlCheck(sqlite3* db, int i);
extern sqlite3* sqlOpen(const std::string filename, int flags);
extern void sqlClose(sqlite3* db);
extern void sqlStmt(sqlite3* db, const char* sql);
extern int sqlGetVersion(sqlite3* db);

extern void sqlPrepareFlux(sqlite3* db);
extern void sqlWriteFlux(sqlite3* db, int track, int side, const Fluxmap& fluxmap);
extern std::unique_ptr<Fluxmap> sqlReadFlux(sqlite3* db, int track, int side);

extern void sqlWriteStringProperty(sqlite3* db, const std::string& name, const std::string& value);
extern std::string sqlReadStringProperty(sqlite3* db, const std::string& name);

extern void sqlWriteIntProperty(sqlite3* db, const std::string& name, long value);
extern long sqlReadIntProperty(sqlite3* db, const std::string& name);

#endif
