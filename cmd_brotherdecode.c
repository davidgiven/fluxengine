#include "globals.h"
#include "sql.h"
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#define TRACK_SYNC_SEQUENCE_LENGTH 53

#define SECTOR_RECORD_ID 0x157
#define END_OF_SECTOR_RECORD_ID 0x2ed
#define DATA_RECORD_ID 0x1db
#define END_OF_DATA_RECORD_ID 0x155

static const char* inputfilename = NULL;
static const char* outputfilename = NULL;
static bool verbose = false;
static sqlite3* indb;
static sqlite3* outdb;

static int cursor;
static struct encoding_buffer* inputbuffer;
static uint32_t inputfifo;

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

static void flush_output_fifo(void)
{
    while (bitcount != 0)
        write_bit(false);
}

static void write_quintet(uint8_t quintet)
{
    for (int i=0; i<5; i++)
    {
        write_bit(quintet & 0x10);
        quintet <<= 1;
    }
}

static void write_byte(uint8_t byte)
{
    flush_output_fifo();
    outputbuffer[outputbufferpos++] = byte;
}

static void write_word(uint16_t word)
{
    write_byte(word & 0xff);
    write_byte(word >> 8);
}

static void log_record(void)
{
    if (verbose)
        printf("\n    % 8.3fms [0x%05x] ",
            (double)elapsed_time / 1e6, cursor);
    else
        putchar('.');
}

static void read_bits(int count)
{
    while (count > 0)
    {
        if (cursor >= inputbuffer->length_pulses)
            break;
        bool bit = inputbuffer->bitmap[cursor++];
        inputfifo = (inputfifo << 1) | bit;
        count--;
    }
    elapsed_time = cursor * inputbuffer->pulselength_ns;
}

static int decode_data_gcr(uint8_t gcr)
{
    switch (gcr)
    {
        case 0x55: return 0; // 00000
        case 0x57: return 1; // 00001
        case 0x5b: return 2; // 00010
        case 0x5d: return 3; // 00011
        case 0x5f: return 4; // 00100 
        case 0x6b: return 5; // 00101
        case 0x6d: return 6; // 00110
        case 0x6f: return 7; // 00111
        case 0x75: return 8; // 01000
        case 0x77: return 9; // 01001
        case 0x7b: return 10; // 01010
        case 0x7d: return 11; // 01011
        case 0x7f: return 12; // 01100
        case 0xab: return 13; // 01101
        case 0xad: return 14; // 01110
        case 0xaf: return 15; // 01111
        case 0xb5: return 16; // 10000
        case 0xb7: return 17; // 10001
        case 0xbb: return 18; // 10010
        case 0xbd: return 19; // 10011
        case 0xbf: return 20; // 10100
        case 0xd5: return 21; // 10101
        case 0xd7: return 22; // 10110
        case 0xdb: return 23; // 10111
        case 0xdd: return 24; // 11000
        case 0xdf: return 25; // 11001
        case 0xeb: return 26; // 11010
        case 0xed: return 27; // 11011
        case 0xef: return 28; // 11100
        case 0xf5: return 29; // 11101
        case 0xf7: return 30; // 11110
        case 0xfb: return 31; // 11111
    }
    return -1;
};

static const uint16_t sector_gcr_table[] = {
       0xDFB5, 0x5B6F, 0x7DF7, 0xBFD5, 0xF57F, 0x6D5D, 0xAFEB, 0xDDB7,
       0x5775, 0x7BFB, 0xBDD7, 0xEFAB, 0x6B5F, 0xADED, 0xDBBB, 0x5577,
       0x77DB, 0xBBAD, 0xED6B, 0x5FEF, 0xABBD, 0xD77B, 0xFB57, 0x75DD,
       0xB7AF, 0xEB6D, 0x5DF5, 0x7FBF, 0xD57D, 0xF75B, 0x6FDF, 0xB5B5,
       0xDF6F, 0x5BF7, 0x7DD5, 0xBF7F, 0xF55D, 0x6DEB, 0xAFB7, 0xDD75,
       0x57FB, 0x7BD7, 0xBDAB, 0xEF5F, 0x6BED, 0xADBB, 0xDB77, 0xBB55,
       0xEDDB, 0x5FAD, 0xAB6B, 0xD7EF, 0xFBBD, 0x757B, 0xB757, 0xEBDD,
       0x5DAF, 0x7F6D, 0xD5F5, 0xF7BF, 0x6F7D, 0xB55B, 0xDFDF, 0x5BB5,
       0x7D6F, 0xBFF7, 0xF5D5, 0x6D7F, 0xAF5D, 0xDDEB, 0x57B7, 0x7B75,
       0xBDFB, 0xEFD7, 0x6BAB, 0xAD5F, 0xDBED, 0x55BB
};

static int decode_sector_gcr(uint16_t gcr)
{
    for (int i=0; i<sizeof(sector_gcr_table)/sizeof(*sector_gcr_table); i++)
    {
        if (sector_gcr_table[i] == gcr)
            return i + 1;
    }
    printf("[unknown sector gcr word 0x%04x]", gcr);
    return -1;
}

static void read_sector_record(void)
{
    read_bits(16);
    int trackno = decode_sector_gcr(inputfifo & 0xffff);
    write_byte(trackno);

    read_bits(16);
    int sectorno = decode_sector_gcr(inputfifo & 0xffff);
    write_byte(sectorno);

    read_bits(10);
    write_word(inputfifo & 0x3ff);

    if (verbose)
        printf("sector [%d, %d]", trackno, sectorno);
}

static void read_data_record(void)
{
    for (int i=0; i<440; i++)
    {
        read_bits(8);
        int gcr = inputfifo & 0xff;
        int data = decode_data_gcr(gcr);
        write_quintet(data);
    }

    read_bits(10);
    write_word(inputfifo & 0x3ff);
}

static void read_record(void)
{
    log_record();
    read_bits(10);
    uint16_t id = inputfifo & 0x3ff;
    outputbufferpos = 0;
    write_word(id);

    switch (id)
    {
        case SECTOR_RECORD_ID:
            read_sector_record();
            break;

        case DATA_RECORD_ID:
            read_data_record();
            break;

        default:
            printf("unknown record %03x\n", inputfifo & 0x3ff);
    }
}

static void decode_track_cb(int track, int side, const struct fluxmap* fluxmap)
{
    printf("Track %02d side %d: ", track, side);

    nanoseconds_t clock_period = fluxmap_guess_clock(fluxmap);
    printf("% 4.1fus ", (double)clock_period/(double)1000);

    inputbuffer = fluxmap_decode(fluxmap, clock_period);

    cursor = 0;
    int record = 0;
    int count = 0;
    while (cursor < inputbuffer->length_pulses)
    {
        bool bit = inputbuffer->bitmap[cursor++];
        if (!bit)
        {
            count = 0;
            continue;
        }

        count++;
        if (count == TRACK_SYNC_SEQUENCE_LENGTH)
        {
            elapsed_time = cursor * inputbuffer->pulselength_ns;
            read_record();
            sql_write_record(outdb, track, side, record++, outputbuffer, outputbufferpos);
            count = 0;
        }
    }

    putchar('\n');

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
