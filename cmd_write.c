#include "globals.h"
#include "sql.h"
#include <unistd.h>

static const char* filename = NULL;
static int start_track = 0;
static int end_track = 79;
static int start_side = 0;
static int end_side = 1;
static sqlite3* db;

static void syntax_error(void)
{
    fprintf(stderr,
        "syntax: fluxclient write <options>:\n"
        "  -o <filename>       input filename\n"
        "  -s <start track>    defaults to 0\n"
        "  -e <end track>      defaults to 79\n"
        "  -0                  read just side 0 (defaults to both)\n" 
        "  -1                  read just side 1 (defaults to both)\n" 
    );
    exit(1);
}

static char* const* parse_options(char* const* argv)
{
	for (;;)
	{
		switch (getopt(countargs(argv), argv, "+o:s:e:01"))
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
    db = sql_open(filename, SQLITE_OPEN_READONLY);
    atexit(close_file);
}

void cmd_write(char* const* argv)
{
    argv = parse_options(argv);
    if (countargs(argv) != 1)
        syntax_error();
    if (!filename)
        error("you must supply a filename to read from");
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

            struct raw_data_buffer buffer;
            buffer.len = sizeof(buffer.buffer);
            if (!sql_read_flux(db, t, side, buffer.buffer, &buffer.len))
                printf("no data in file\n");
            else
                usb_write(side, &buffer);
        }
    }
}

