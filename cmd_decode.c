#include "globals.h"
#include "sql.h"
#include <unistd.h>
#include <string.h>

static const char* inputfilename = NULL;
static const char* outputfilename = NULL;
static sqlite3* indb;
static sqlite3* outdb;

static uint32_t buckets[128];
static uint8_t outputbuffer[100*1024];
static int outputbufferpos;
static uint8_t fifo = 0;
static int bitcount = 0;
static int unsynced = false;

static void syntax_error(void)
{
    fprintf(stderr, "syntax: fluxclient decode -i <inputfilename> -o <outputfilename> \n");
    exit(1);
}

static char* const* parse_options(char* const* argv)
{
	for (;;)
	{
		switch (getopt(countargs(argv), argv, "+i:o:"))
		{
			case -1:
				return argv + optind - 1;

			case 'i':
                inputfilename = optarg;
                break;

            case 'o':
                outputfilename = optarg;
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
    atexit(close_files);
}

static int find_peak(int* cursor)
{
    while (*cursor < 128)
    {
        if (buckets[*cursor] > 100)
        {
            /* This is high enough that it's probably the start of a spike.
             * Find the end, tracking the highest value. */

            int maxindex = *cursor;
            int maxvalue = 0;
            while (*cursor < 128)
            {
                int v = buckets[*cursor];
                if (v < 10)
                    break;
                if (v > maxvalue)
                {
                    maxvalue = v;
                    maxindex = *cursor;
                }

                (*cursor)++;
            }
            return maxindex;
        }

        (*cursor)++;
    }
    return -1;
}

static void queue_bit(bool bit)
{
    fifo <<= 1;
    fifo |= bit;

    if (unsynced && (fifo == 0x4e))
    {
        bitcount = 7;
        unsynced = false;
    }

    if (bitcount == 7)
    {
        outputbuffer[outputbufferpos++] = fifo;
        bitcount = 0;
    }
    else
        bitcount++;
}

static void decode_track_cb(int track, int side, const uint8_t* data, size_t len)
{
    printf("Track %d side %d: ", track, side);

    memset(buckets, 0, sizeof(buckets));
    for (int i=0; i<len; i++)
        buckets[data[i] & 0x7f]++;

    int cursor = 0;
    int peak1 = find_peak(&cursor);
    int peak2 = find_peak(&cursor);
    int peak3 = find_peak(&cursor);
    int peak4 = find_peak(&cursor);
    if ((peak3 == -1) || (peak4 != -1))
        error("don't understand disk (something's wrong with the encoding)");

    double freq = (double)TICK_FREQUENCY / (double)(peak3 - peak1);
    int split1 = (peak1 + peak2) / 2;
    int split2 = (peak2 + peak3) / 2;
    int split3 = peak3 + (split2 - split1);
    printf("freq %d kHz\n", (int)(freq/1000.0));

    /* 
     * MFM divides the signal into two-bit cells, which can be either x0
     * (representing 0) and 01 (representing 1). x can be any value; the rules
     * set it to 0 of the previous cell contained 01, and 1 otherwise.
     * 
     * However, all we have are the intervals and so we don't know where a cell
     * begins. Consider:
     * 
     * Data:    1  1  0  0  0  1  0  0  1  1  1  0  1  0  1  0  1  0
     * Signal: 01 01 00 10 10 01 00 10 01 01 01 00 01 00 01 00 01 00
     * 
     * If our signal is offset half a cell, we get this:
     * 
     * Data:      0  0  1  1  0  0  1  0  0  0  0  0  0  0  0  0  0
     * Signal: 0 10 10 01 01 00 10 01 00 10 10 10 00 10 00 10 00 10 0
     * 
     * However! This violates the rules. We have 10 00. Both of these
     * encode zeros, which means the second 00 should have been a 10.
     * So, after a long transition, we know that the cell sequence
     * must have been (assuming correct encoding) 00 01, encoding 0
     * and 1, and that the next interval will be at the start of a
     * cell.
     * 
     * Data:      0  0  1  1  0  0  1  0  0  0  0  0 |  1  0  1  0  1  0
     * Signal: 0 10 10 01 01 00 10 01 00 10 10 10 00 | x1 00 01 00 01 00
     *                                               \ resync
     * 
     * But there's more. Even after we get the bitstream, we don't know
     * where byte boundaries are. So, certain values are written to the
     * disk with missing bits:
     * 
     * Data: C2    1  1  0  0  0  0  1  0
     * Should be: 01 01 00 10 10 10 01 10
     * Actually:  01 01 00 10 00 10 01 10
     *                        ^ wrong
     * 
     * What'll happen is that we'll resync in the wrong place and all our data
     * is gibberish. So we need to explicitly check the fifo for sync bytes and
     * cope.
     */

    outputbufferpos = 0;
    unsynced = true;
    bool phase = false;
    for (int i=0; i<len; i++)
    {
        int t = data[i];
        if ((t & 0x80) || (t > split3))
        {
            unsynced = true;
            continue;
        }

        if (t > split2)
        {
            /* Three zeroes.
             *  1,00,01
             *    ^^ ^^
             */
            queue_bit(false);
            if ((fifo & 0x0f) == 0x0c)
            {
                /* This is a sync byte. */
                phase = true;
            }
            else
            {
                /* Not a sync byte --- resync. */

                phase = false;
            }
            queue_bit(!phase);
        }
        else if (t < split1)
        {
            /* One zero: either 1,01, or ,10,1. */
            queue_bit(!phase);
        }
        else
        {
            /* Two zeroes. */
            if (!phase)
            {
                /*
                 * 1,00,1
                 *   ^^
                 */
                queue_bit(false);
                phase = true;
            }
            else
            {
                /* 
                 * ,10,01,
                 *   ^ ^
                 */
                queue_bit(false);
                queue_bit(true);
                phase = false;
            }
        }
    }

    /* Flush. */
    for (int i=0; i<8; i++)
        queue_bit(false);

    FILE* fp = fopen("test.dat", "wb");
    fwrite(outputbuffer, 1, outputbufferpos, fp);
    fclose(fp);
}

void cmd_decode(char* const* argv)
{
    argv = parse_options(argv);
    if (countargs(argv) != 1)
        syntax_error();
    if (!inputfilename)
        error("you must supply a filename to read from");

    open_files();
    sql_for_all_raw_data(indb, decode_track_cb);
}

