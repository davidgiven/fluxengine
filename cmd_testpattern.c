#include "globals.h"
#include "sql.h"
#include <unistd.h>

static const char* filename = NULL;
static int start_track = 0;
static int end_track = 79;
static int start_side = 0;
static int end_side = 1;
static int track_length_ms = 200;
static int long_pulse_us = 6;
static int short_pulse_us = 4;
static sqlite3* db;

static void syntax_error(void)
{
    fprintf(stderr,
        "syntax: fluxclient testpattern <options>:\n"
        "  -o <filename>       output filename\n"
        "  -s <start track>    defaults to 0\n"
        "  -e <end track>      defaults to 79\n"
        "  -l <track length>   track length in ms (defaults to 200)\n"
        "  -L <pulse width>    length of long pulse, in microseconds\n"
        "  -S <pulse width>    length of short pulse, in microseconds\n"
        "  -0                  write just side 0 (defaults to both)\n" 
        "  -1                  write just side 1 (defaults to both)\n" 
    );
    exit(1);
}

static char* const* parse_options(char* const* argv)
{
	for (;;)
	{
		switch (getopt(countargs(argv), argv, "+o:s:e:l:01"))
		{
			case -1:
				return argv + optind - 1;

			case 'o':
                filename = optarg;
                break;

            case 's':
                start_track = atoi(optarg);
                break;

            case 'e':
                end_track = atoi(optarg);
                break;

            case 'l':
                track_length_ms = atoi(optarg);
                break;

            case '0':
                start_side = end_side = 0;
                break;

            case '1':
                start_side = end_side = 1;
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
    db = sql_open(filename, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    atexit(close_file);
    sql_prepare_flux(db);
}

static void write_pulsetrain(struct encoding_buffer* buffer, int cursor_ms, int length_ms, int width_us)
{
    int cursor_us = cursor_ms*1000;
    int length_us = length_ms*1000;
    while (length_us > 0)
    {
        encoding_buffer_pulse(buffer, cursor_us);
        length_us -= width_us;
        cursor_us += width_us;
    }
}

void cmd_testpattern(char* const* argv)
{
    argv = parse_options(argv);
    if (countargs(argv) != 1)
        syntax_error();
    if (!filename)
        error("you must supply a filename to write to");
    if (start_track > end_track)
        error("writing to track %d to track %d makes no sense", start_track, end_track);

    struct encoding_buffer* buffer = create_encoding_buffer(track_length_ms*1000);
    
    int cursor_ms = 0;
    while (cursor_ms < track_length_ms)
    {
        write_pulsetrain(buffer, cursor_ms, 1, long_pulse_us);
        cursor_ms++;
        write_pulsetrain(buffer, cursor_ms, 1, short_pulse_us);
        cursor_ms++;
    }

    struct fluxmap* fluxmap = encoding_buffer_encode(buffer);

    open_file();
    sql_stmt(db, "BEGIN;");
    for (int t=start_track; t<=end_track; t++)
    {
        for (int side=start_side; side<=end_side; side++)
        {
            printf("Track %02d side %d: ", t, side);
            fflush(stdout);

            sql_write_flux(db, t, side, fluxmap);
            printf("%d ms in %d bytes\n", fluxmap->length_us / 1000, fluxmap->bytes);
        }
    }
    sql_stmt(db, "COMMIT;");

    free_fluxmap(fluxmap);
    free_encoding_buffer(buffer);
}

