#include "globals.h"
#include "sql.h"
#include "fluxmap.h"
#include <unistd.h>

static const char* filename = NULL;
static int start_track = 0;
static int end_track = 79;
static int start_side = 0;
static int end_side = 1;
static int precompensation_ticks = 1;
static sqlite3* db;

static void syntax_error(void)
{
    fprintf(stderr,
        "syntax: fluxclient write <options>:\n"
        "  -i <filename>       input filename\n"
        "  -s <start track>    defaults to 0\n"
        "  -e <end track>      defaults to 79\n"
        "  -0                  write just side 0 (defaults to both)\n" 
        "  -1                  write just side 1 (defaults to both)\n" 
        "  -P <amount>         amount of write precompensation, in ticks (defaults to 2)\n"
    );
    exit(1);
}

static char* const* parse_options(char* const* argv)
{
	for (;;)
	{
		switch (getopt(countargs(argv), argv, "+i:Ts:e:01P:"))
		{
			case -1:
				return argv + optind - 1;

			case 'i':
                filename = optarg;
                break;

            case 's':
                start_track = atoi(optarg);
                break;

            case 'e':
                end_track = atoi(optarg);
                break;

            case '0':
                start_side = end_side = 0;
                break;

            case '1':
                start_side = end_side = 1;
                break;

            case 'P':
                precompensation_ticks = atoi(optarg);
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
    db = sql_open(filename, SQLITE_OPEN_READONLY);
    atexit(close_file);
}

void cmd_write(char* const* argv)
{
    argv = parse_options(argv);
    if (countargs(argv) != 1)
        syntax_error();
    if (!filename)
        error("you must specify an input filename");
    if (start_track > end_track)
        error("writing to track %d to track %d makes no sense", start_track, end_track);

    open_file();
    for (int t=start_track; t<=end_track; t++)
    {
        for (int side=start_side; side<=end_side; side++)
        {
            printf("Track %02d side %d: ", t, side);
            fflush(stdout);
            usb_seek(t);

            struct fluxmap* fluxmap = sql_read_flux(db, t, side);
            if (!fluxmap)
            {
                printf("(no data)\n");
                continue;
            }

            printf("sent %dms", fluxmap->length_us/1000);
            fflush(stdout);

            fluxmap_precompensate(fluxmap, PRECOMPENSATION_THRESHOLD_TICKS, precompensation_ticks);
            usb_write(side, fluxmap);
            printf("\n");
        }
    }
}

