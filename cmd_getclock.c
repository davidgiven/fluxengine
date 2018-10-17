#include "globals.h"
#include "sql.h"
#include <unistd.h>

static const char* input_filename = NULL;
static int track = 0;
static int side = 0;
static int threshold = 100;
static sqlite3* db;

static void syntax_error(void)
{
    fprintf(stderr,
        "syntax: fluxclient getclock <options>:\n"
        "  -i <filename>       input filename (.flux)\n"
        "  -t <track>          track to analyse\n"
        "  -0                  analyse side 0\n"
        "  -1                  analyse side 1\n"
        "  -T <threshold>      noise threshold (default: 100)\n"
    );
    exit(1);
}

static char* const* parse_options(char* const* argv)
{
	for (;;)
	{
		switch (getopt(countargs(argv), argv, "+i:t:01T:"))
		{
			case -1:
				return argv + optind - 1;

            case 'i':
                input_filename = optarg;
                break;

            case 't':
                track = atoi(optarg);
                break;

            case '0':
                side = 0;
                break;

            case '1':
                side = 1;
                break;

            case 'T':
                threshold = atoi(optarg);
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
    db = sql_open(input_filename, SQLITE_OPEN_READONLY);
    atexit(close_file);
}

void cmd_getclock(char* const* argv)
{
    argv = parse_options(argv);
    if (countargs(argv) != 1)
        syntax_error();
    if (!input_filename)
        error("you must supply a filename to read from");

    open_file();
    struct fluxmap* fluxmap = sql_read_flux(db, track, side);
    if (!fluxmap)
        error("no data for that track in the file");
    
    uint32_t buckets[256] = {};
    for (int i=0; i<fluxmap->bytes; i++)
        buckets[fluxmap->intervals[i]]++;

    bool skipping = false;
    for (int i=0; i<256; i++)
    {
        uint32_t v = buckets[i];
        if (v < threshold)
        {
            if (!skipping)
            {
                skipping = true;
                printf("...\n");
            }
        }
        else
        {
            skipping = false;
            printf("(0x%02x) % 6.2fus: %d\n", i, (double)i / (double)TICKS_PER_US, v);
        }
    }

    nanoseconds_t estimated_clock = fluxmap_guess_clock(fluxmap);
    printf("Estimated clock: %6.2fus\n", estimated_clock/1000.0);
}

