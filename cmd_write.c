#include "globals.h"
#include "sql.h"
#include <unistd.h>

static const char* filename = NULL;
static bool test_pattern = false;
static int start_track = 0;
static int end_track = 79;
static int start_side = 0;
static int end_side = 1;
static sqlite3* db;

static void syntax_error(void)
{
    fprintf(stderr,
        "syntax: fluxclient write <options>:\n"
        "  -o <filename>       input filename (can't use with -T)\n"
        "  -T                  write a test pattern (can't use with -o)\n"
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
		switch (getopt(countargs(argv), argv, "+o:Ts:e:01"))
		{
			case -1:
				return argv + optind - 1;

			case 'o':
                filename = optarg;
                break;

            case 'T':
                test_pattern = true;
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

static void write_data(struct raw_data_buffer* buffer, int ticks, int* cursor, uint8_t data)
{
    while ((ticks > 0) && (*cursor < buffer->len))
    {
        buffer->buffer[(*cursor)++] = data;
        ticks -= data;
    }
}

static void create_test_pattern(struct raw_data_buffer* buffer)
{
    int cursor = 0;
    const int five_milliseconds = 5 * TICK_FREQUENCY / 1000;
    int step = 0x38;

    while (cursor < buffer->len)
    {
        write_data(buffer, five_milliseconds, &cursor, step);
        write_data(buffer, five_milliseconds, &cursor, 0x30);
        if (step < 0xf8)
            step += 8;
    }
}

void cmd_write(char* const* argv)
{
    argv = parse_options(argv);
    if (countargs(argv) != 1)
        syntax_error();
    if (!!filename == test_pattern)
        error("you must specify a filename to read from, or a test pattern");
    if (start_track > end_track)
        error("writing to track %d to track %d makes no sense", start_track, end_track);

    if (filename)
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
            if (filename)
            {
                if (!sql_read_flux(db, t, side, buffer.buffer, &buffer.len))
                    continue;
            }
            if (test_pattern)
                create_test_pattern(&buffer);

            buffer.len &= ~(FRAME_SIZE-1);

            uint32_t time = 0;
            for (int i=0; i<buffer.len; i++)
                time += buffer.buffer[i];
            printf("sent %dms", (time*1000)/TICK_FREQUENCY);
            fflush(stdout);

            int bytes_actually_written = usb_write(side, &buffer);

            time = 0;
            for (int i=0; i<bytes_actually_written; i++)
                time += buffer.buffer[i];
            printf(", wrote %dms\n", (time*1000)/TICK_FREQUENCY);
        }
    }
}

