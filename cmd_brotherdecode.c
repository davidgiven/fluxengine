#include "globals.h"
#include "sql.h"
#include <unistd.h>
#include <string.h>

#define CLOCK_LOCK_BOOST 6 /* arbitrary */
#define CLOCK_LOCK_DECAY 1 /* arbitrary */
#define CLOCK_DETECTOR_AMPLITUDE_THRESHOLD 60 /* arbi4rary */
#define CLOCK_ERROR_BOUNDS 0.25

#define IAM 0xFC   /* start-of-track record */
#define IAM_LEN    4
#define IDAM 0xFE  /* sector header */
#define IDAM_LEN   10
#define DAM1 0xF8  /* sector data (type 1) */
#define DAM2 0xFB  /* sector data (type 2) */
#define DAM_LEN    6 /* plus user data */
/* Length of a DAM record is determined by the previous sector header. */

static const char* inputfilename = NULL;
static const char* outputfilename = NULL;
static bool verbose = false;
static sqlite3* indb;
static sqlite3* outdb;

static int cursor;
static nanoseconds_t elapsed_time;
static uint8_t outputbuffer[20*1024];
static int outputbufferpos;
static uint8_t outputfifo = 0;
static int bitcount = 0;
static bool phase = false;

static void syntax_error(void)
{
    fprintf(stderr,
        "syntax: fluxclient mfmdecode <options>:\n"
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

static void write_bit(bool bit)
{
    outputfifo = (outputfifo << 1) | bit;
    bitcount++;
    if (bitcount == 8)
    {
        outputbuffer[outputbufferpos++] = outputfifo;
        bitcount = 0;
    }
}

static void log_record(void)
{
    if (verbose)
        printf("\n    % 8.3fms [0x%05x] ",
            (double)elapsed_time / 1e6, cursor);
    else
        putchar('.');
}

static void decode_track_cb(int track, int side, const struct fluxmap* fluxmap)
{
    printf("Track %02d side %d: ", track, side);

    nanoseconds_t clock_period = fluxmap_guess_clock(fluxmap);
    printf("% 4.1fus ", (double)clock_period/(double)1000);

    struct encoding_buffer* decoded = fluxmap_decode(fluxmap, clock_period);

    for (int i=0; i<decoded->length_pulses; i++)
        putchar(decoded->bitmap[i] ? 'X' : '.');

#if 0
    cursor = 0;
    uint64_t inputfifo = 0;
    bool reading = false;
    int record = 0;

    while (cursor < decoded->length_pulses)
    {
        elapsed_time = cursor * decoded->pulselength_ns;
        bool bit = decoded->bitmap[cursor++];
        inputfifo = (inputfifo << 1) | bit;

        /* 
         * The IAM record, which is the first one on the disk (and is optional), uses
         * a distorted 0xC2 0xC2 0xC2 marker to identify it. Unfortunately, if this is
         * shifted out of phase, it becomes a legal encoding, so if we're looking at
         * real data we can't honour this.
         * 
         * 0xC2 is:
         * data:    1  1  0  0  0  0  1 0
         * mfm:     01 01 00 10 10 10 01 00 = 0x5254
         * special: 01 01 00 10 00 10 01 00 = 0x5224
         *                    ^^^^
         * shifted: 10 10 01 00 01 00 10 0. = legal, and might happen in real data
         * 
         * Therefore, when we've read the marker, the input fifo will contain
         * 0xXXXX522252225222.
         * 
         * All other records use 0xA1 as a marker:
         * 
         * 0xA1  is:
         * data:    1  0  1  0  0  0  0  1
         * mfm:     01 00 01 00 10 10 10 01 = 0x44A9
         * special: 01 00 01 00 10 00 10 01 = 0x4489
         *                       ^^^^^
         * shifted: 10 00 10 01 00 01 00 1
         * 
         * When this is shifted out of phase, we get an illegal encoding (you
         * can't do 10 00). So, if we ever see 0x448944894489 in the input
         * fifo, we know we've landed at the beginning of a new record.
         */
         
        uint64_t masked = inputfifo & 0xFFFFFFFFFFFFLL;
        if ((!reading && (masked == 0x522452245224LL)) || (masked == 0x448944894489LL))
        {
            log_record();

            if (reading)
                sql_write_record(outdb, track, side, record++, outputbuffer, outputbufferpos);

            if (!reading)
                memcpy(outputbuffer, "\xc2\xc2\xc2", 3);
            else
                memcpy(outputbuffer, "\xa1\xa1\xa1", 3);

            reading = true;
            outputbufferpos = 3;
            bitcount = 0;
            phase = 0;
        }
        else if (reading)
        {
            if (phase)
                write_bit(bit);
            phase = !phase;
        }
    }

    if (reading)
        sql_write_record(outdb, track, side, record++, outputbuffer, outputbufferpos);

    if (verbose)
        printf("\n    ");
    printf(" = %d records\n", record);
#endif
}

void cmd_brotherdecode(char* const* argv)
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
