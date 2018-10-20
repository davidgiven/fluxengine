#include "globals.h"
#include "sql.h"
#include "fluxmap.h"
#include <unistd.h>

#define DATA_PULSE_US 4
#define GAP_PULSE_US 6

#define REPETITIONS 300

static int track = 0;
static double spacing_us = 2;
static int precompensation_ticks = 1;

static void syntax_error(void)
{
    fprintf(stderr,
        "syntax: fluxclient calibrate <options>:\n"
        "  -t <track>          track to write to\n"
        "  -p <spacing>        pulse spacing (in us)\n"
        "  -P <amount>         amount of precompensation (in ticks)\n"
    );
    exit(1);
}

static char* const* parse_options(char* const* argv)
{
	for (;;)
	{
		switch (getopt(countargs(argv), argv, "+t:p:P:"))
		{
			case -1:
				return argv + optind - 1;

			case 't':
                track = atoi(optarg);
                break;

            case 'p':
                spacing_us = atof(optarg);
                break;

            case 'P':
                precompensation_ticks = atoi(optarg);
                break;

			default:
				syntax_error();
		}
	}
}

static void append_test_pattern(struct fluxmap* fluxmap)
{
    uint8_t t = DATA_PULSE_US * TICKS_PER_US;
    for (int i=0; i<100; i++)
        fluxmap_append_intervals(fluxmap, &t, 1);

    /* left pulse */
    t = GAP_PULSE_US * TICKS_PER_US;
    fluxmap_append_intervals(fluxmap, &t, 1);

    /* middle and right pulses */
    t = spacing_us * TICKS_PER_US;
    fluxmap_append_intervals(fluxmap, &t, 1);
    fluxmap_append_intervals(fluxmap, &t, 1);

    /* then the gap followed by a trailing data pulse */
    t = GAP_PULSE_US * TICKS_PER_US;
    fluxmap_append_intervals(fluxmap, &t, 1);
}

static void find_test_pattern(struct fluxmap* fluxmap, int* cursor, int* accumulator)
{
    int gap_threshold = GAP_PULSE_US * TICKS_PER_US * 0.80;

    for (;;)
    {
        if (*cursor >= fluxmap->bytes)
            return;
        uint8_t t = fluxmap->intervals[(*cursor)++];
        if (t > gap_threshold)
            break;
    }

    if (*cursor >= fluxmap->bytes)
        return;
    *accumulator += fluxmap->intervals[(*cursor)++];
    if (*cursor >= fluxmap->bytes)
        return;
    *accumulator += fluxmap->intervals[(*cursor)++];
    (*cursor)++;
}

static double read_and_calculate_error(void)
{
    struct fluxmap* input = usb_read(0, 1);
    int cursor = 0;
    fluxmap_seek_clock(input, &cursor, 16);
    int accumulator = 0;
    for (int i=0; i<REPETITIONS; i++)
        find_test_pattern(input, &cursor, &accumulator);
    double real_spacing_us = (double)accumulator / (double)(REPETITIONS * 2 * TICKS_PER_US);
    double e = (real_spacing_us - spacing_us) / spacing_us;
    free_fluxmap(input);
    return e;
}

void cmd_calibrate(char* const* argv)
{
    argv = parse_options(argv);
    if (countargs(argv) != 1)
        syntax_error();

    int track_ms = usb_measure_speed();
    printf("Each track is %dms long.\n", track_ms);

    struct fluxmap* output = create_fluxmap();
    for (int i=0; i<REPETITIONS; i++)
        append_test_pattern(output);
    printf("Test pattern is %dms long.\n", output->length_us/1000);

    printf("Writing...\n");
    usb_seek(track);
    usb_write(0, output);

    printf("Reading...\n");
    double e = read_and_calculate_error();
    printf("Error: %f\n", e);

    printf("Writing...\n");
    usb_seek(track);
    struct fluxmap* compensated = copy_fluxmap(output);
    fluxmap_precompensate(compensated, PRECOMPENSATION_THRESHOLD_TICKS, precompensation_ticks);
    usb_write(0, compensated);
    free_fluxmap(compensated);

    printf("Reading...\n");
    e = read_and_calculate_error();
    printf("Error: %f\n", e);

}


