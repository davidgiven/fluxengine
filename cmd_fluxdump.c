#include "globals.h"
#include "sql.h"
#include <unistd.h>

static const char* input_filename = NULL;
static const char* output_filename = NULL;
static int track = 0;
static int side = 0;
static int resolution_ns = NS_PER_TICK;
static sqlite3* db;

static void syntax_error(void)
{
    fprintf(stderr,
        "syntax: fluxclient fluxdump <options>:\n"
        "  -i <filename>       input filename (.flux)\n"
        "  -o <filename>       output filename (.raw)\n"
        "  -t <track>          track to dump\n"
        "  -0                  dump side 0\n"
        "  -1                  dump side 1\n"
        "  -r <resolution>     time per sample in ns (default: one tick)\n"
    );
    exit(1);
}

static char* const* parse_options(char* const* argv)
{
	for (;;)
	{
		switch (getopt(countargs(argv), argv, "+i:o:t:01r:L"))
		{
			case -1:
				return argv + optind - 1;

            case 'i':
                input_filename = optarg;
                break;

			case 'o':
                output_filename = optarg;
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

            case 'r':
                resolution_ns = atoi(optarg);
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

void cmd_fluxdump(char* const* argv)
{
    argv = parse_options(argv);
    if (countargs(argv) != 1)
        syntax_error();
    if (!input_filename)
        error("you must supply a filename to read from");
    if (!output_filename)
        error("you must supply a filename to write to");
    if (resolution_ns < NS_PER_TICK)
        error("resolution too fine (minimum: %dns)", NS_PER_TICK);

    open_file();
    struct fluxmap* fluxmap = sql_read_flux(db, track, side);
    if (!fluxmap)
        error("no data for that track in the file");
    
    printf("Length: %d ms\n", fluxmap->length_us / 1000);
    int length_r = fluxmap->length_us * 1000 / resolution_ns;
    printf("Output size: %d bytes\n", length_r);

    uint8_t* buffer = calloc(1, length_r);
    int cursor_ns = 0;
    for (int i=0; i<fluxmap->bytes; i++)
    {
        uint8_t t = fluxmap->intervals[i];
        cursor_ns += t * NS_PER_TICK / resolution_ns;
        if (cursor_ns < length_r)
            buffer[cursor_ns] = 0x7f;
    }

    FILE* fp = fopen(output_filename, "wb");
    fwrite(buffer, length_r, 1, fp);
    fclose(fp);
    free(buffer);
}

