#include "globals.h"
#include "sql.h"
#include <unistd.h>

static const char* filename = NULL;
static int start_track = 0;
static int end_track = 79;
static sqlite3* db;

static void syntax_error(void)
{
    fprintf(stderr, "syntax: fluxclient read -f <filename>\n");
    exit(1);
}

static char* const* parse_options(char* const* argv)
{
	for (;;)
	{
		switch (getopt(countargs(argv), argv, "+f:s:e:"))
		{
			case -1:
				return argv + optind - 1;

			case 'f':
                filename = optarg;
                break;

            case 's':
                start_track = atoi(optarg);
                break;

            case 'e':
                end_track = atoi(optarg);
                break;

			default:
				syntax_error();
		}
	}
}

static void close_file(void)
{
    sqlite3_close(db);
}

static void open_file(void)
{
    db = sql_open(filename);
    atexit(close_file);

    sql_stmt(db, "PRAGMA synchronous = OFF;");
    sql_stmt(db, "CREATE TABLE IF NOT EXISTS rawdata ("
                 "  track INTEGER,"
                 "  side INTEGER,"
                 "  data BLOB,"
                 "  PRIMARY KEY(track, side)"
                 ");");
}

void cmd_read(char* const* argv)
{
    argv = parse_options(argv);
    if (countargs(argv) != 1)
        syntax_error();
    if (!filename)
        error("you must supply a filename to write to");
    if (start_track > end_track)
        error("reading from track %d to track %d makes no sense", start_track, end_track);

    open_file();
    sql_stmt(db, "BEGIN;");

    sqlite3_stmt* stmt;
    sql_check(db, sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO rawdata (track, side, data) VALUES (:track, :side, :data)",
        -1, &stmt, NULL));
    sql_bind_int(db, stmt, ":side", 0);

    for (int t=start_track; t<=end_track; t++)
    {
        printf("Track %02d: ", t);
        fflush(stdout);
	    usb_seek(t);

        struct raw_data_buffer buffer;
	    usb_read(0, &buffer);

        sql_bind_int(db, stmt, ":track", t);
        sql_bind_blob(db, stmt, ":data", &buffer.buffer, buffer.len);
        if (sqlite3_step(stmt) != SQLITE_DONE)
            error("failed to write to database: %s", sqlite3_errmsg(db));
        sql_check(db, sqlite3_reset(stmt));

        printf("%d bytes\n", buffer.len);
    }
    sql_stmt(db, "COMMIT;");
}
