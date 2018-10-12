#include "globals.h"
#include "sql.h"
#include <unistd.h>
#include <string.h>

#define DEBOUNCE_TICKS 0x28

#define CLOCK_LOCK_BOOST 6 /* arbitrary */
#define CLOCK_LOCK_DECAY 1 /* arbitrary */
#define CLOCK_DETECTOR_AMPLITUDE_THRESHOLD 60 /* arbi4rary */
#define CLOCK_ERROR_BOUNDS 0.50

static const char* inputfilename = NULL;
static const char* outputfilename = NULL;
static bool verbose = false;
static sqlite3* indb;
static sqlite3* outdb;

static const uint8_t* inputbuffer;
static int inputlen;
static int cursor;
static int elapsed_ticks;

static int clock_period; /* mfm cell width */
static int period0; /* lower limit for a short transition */
static int period1; /* between short and long transitions */
static int period2; /* upper limit for a long transition */

static uint8_t outputbuffer[5*1024];
static int outputbufferpos;
static uint8_t fifo = 0;
static int bitcount = 0;
static bool phaselocked = false;

static int thislength = 0;
static int nextlength = 0;

static void syntax_error(void)
{
    fprintf(stderr,
        "syntax: fluxclient fmdecode <options>:\n"
        "  -i <filename>       input filename (.flux)\n"
        "  -o <filename>       output filename (.rec)\n"
        "  -v                  verbose decoding\n"
    );
    exit(1);
}

static char* const* parse_options(char* const* argv)
{
	for (;;)
	{
		switch (getopt(countargs(argv), argv, "+i:o:v"))
		{
			case -1:
				return argv + optind - 1;

			case 'i':
                inputfilename = optarg;
                break;

            case 'o':
                outputfilename = optarg;
                break;

            case 'v':
                verbose = true;
                break;

			default:
				syntax_error();
		}
	}
}

static void close_files(void)
{
    if (indb)
        sqlite3_close(indb);
    if (outdb)
        sqlite3_close(outdb);
}

static void open_files(void)
{
    indb = sql_open(inputfilename, SQLITE_OPEN_READONLY);
    outdb = sql_open(outputfilename, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    sql_prepare_record(outdb);
    atexit(close_files);
}

static void queue_bit(bool bit)
{
    fifo <<= 1;
    fifo |= bit;
    bitcount++;
}

static bool read_bit(void)
{
    /*
     * FM is incredibly stupid. Every cell has a pulse at the beginning;
     * cells which contain a 1 have a pulse in the middle; cells which don't
     * have a 0. Therefore we have these two states:
     * 
     * Data:    0   1
     * Signal: 10  11
     * 
     * Therefore, a *long* interval represents a 0, and two *short* intervals
     * represent a 1.
     */

    if (cursor >= inputlen)
        return false;
    uint8_t t = inputbuffer[cursor++];
    elapsed_ticks += t;

    if ((t < period0) || (t > period2))
    {
        /* Garbage data: our clock's wrong. */
        phaselocked = false;
        return false;
    }
    else if (t < period1)
    {
        /* That was the first short interval of a 1: now consume any additional
         * until we reach the clock pulse again. */
        if (cursor >= inputlen)
            return false;
        while (t < period1)
        {
            if (cursor >= inputlen)
                return false;
            uint8_t tt = inputbuffer[cursor++];
            elapsed_ticks += tt;
            t += tt;
        }
        
        /* If the clock pulse didn't show up, give up. */
        if (t > period2)
        {
            phaselocked = false;
            return false;
        }
        return true;
    }
    else
    {
        /* A long transition. */
        return false;
    }
}

static uint8_t read_byte(void)
{
    while (phaselocked && (bitcount < 8))
        queue_bit(read_bit());
    bitcount = 0;
    return fifo;
}

static void log_record(char type)
{
    if (verbose)
        printf("\n    % 8.3fms [0x%05x]: ",
            (double)elapsed_ticks / (TICKS_PER_US*1000.0), cursor);
    putchar(type);
}

static bool process_byte(uint8_t b)
{
    outputbuffer[outputbufferpos++] = b;
    if (outputbufferpos == sizeof(outputbuffer))
        goto abandon_record;

    error("unimplemented --- can't handle FM yet");

abandon_record:
    if (verbose && (outputbufferpos > 4))
        printf(" misread");
    phaselocked = false;
    return false;
}

static void decode_track_cb(int track, int side, const struct fluxmap* fluxmap)
{
    printf("Track %02d side %d: ", track, side);

    inputbuffer = fluxmap->intervals;
    inputlen = fluxmap->bytes;
    cursor = 0;
    elapsed_ticks = 0;
    int record = 0;

    while (cursor < inputlen)
    {
        if (!phaselocked)
        {
            while (cursor < inputlen)
            {
                clock_period = fluxmap_seek_clock(fluxmap, &cursor, 16);

                /* 
                * Okay, this looks good. We'll assume this is clock/2 --- 250kHz
                * for HD floppies; this is one long transition.
                */

                double short_time = clock_period / 2.0;
                double long_time = short_time * 2.0;

                period0 = short_time - short_time * CLOCK_ERROR_BOUNDS;
                period1 = (short_time + long_time) / 2.0;
                period2 = long_time + long_time * CLOCK_ERROR_BOUNDS;
                phaselocked = true;

                while (phaselocked && (cursor < inputlen))
                {
                    if (read_bit())
                        goto found_byte;
                }
            }
        found_byte:;
            fifo = 1;
            bitcount = 1;
            outputbufferpos = 0;
        }

        if (process_byte(read_byte()))
        {
            sql_write_record(outdb, track, side, record, outputbuffer, outputbufferpos);
            record++;
        }
    }

    if (verbose)
        printf("\n    ");
    printf(" = %d records\n", record);
}

void cmd_fmdecode(char* const* argv)
{
    argv = parse_options(argv);
    if (countargs(argv) != 1)
        syntax_error();
    if (!inputfilename)
        error("you must supply a filename to read from");
    if (!outputfilename)
        error("you must supply a filename to write to");

    open_files();
    sql_stmt(outdb, "BEGIN");
    sql_stmt(outdb, "DELETE FROM records");
    sql_for_all_flux_data(indb, decode_track_cb);
    sql_stmt(outdb, "COMMIT");
}

