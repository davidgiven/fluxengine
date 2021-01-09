#include "globals.h"
#include "flags.h"
#include "usb/usb.h"
#include "dataspec.h"
#include "fluxmap.h"
#include "decoders/fluxmapreader.h"
#include "writer.h"
#include "protocol.h"
#include "fmt/format.h"
#include <fstream>

static FlagGroup flags = {
	&usbFlags,
};

static DataSpecFlag dest(
    { "--dest", "-d" },
    "destination to analyse",
    ":d=0:t=0:s=0");

static DoubleFlag minInterval(
	{ "--min-interval-us" },
	"Minimum pulse interval",
	2.0);

static DoubleFlag maxInterval(
	{ "--max-interval-us" },
	"Maximum pulse interval",
	10.0);

static DoubleFlag intervalStep(
	{ "--interval-step-us" },
	"Interval step, approximately",
	0.2);

static StringFlag writeCsv(
	{ "--write-csv" },
	"Write detailed CSV data",
	"driveresponse.csv");

int mainAnalyseDriveResponse(int argc, const char* argv[])
{
    flags.parseFlags(argc, argv);

    FluxSpec spec(dest);
	if (spec.locations.size() != 1)
		Error() << "the destination dataspec must contain exactly one track (two sides count as two tracks)";

    usbSetDrive(spec.drive, false, F_INDEX_REAL);
	usbSeek(spec.locations[0].track);

	std::cout << "Measuring rotational speed...\n";
    nanoseconds_t period = usbGetRotationalPeriod(0);
	if (period == 0)
		Error() << "Unable to measure rotational speed (try fluxengine rpm).";

	std::ofstream csv;
	if (writeCsv.get() != "")
		csv.open(writeCsv);

	for (double interval = minInterval; interval<maxInterval; interval += intervalStep)
	{
		unsigned ticks = (unsigned) (interval * TICKS_PER_US);
		std::cout << fmt::format("Interval {:.2f}: ", ticks * US_PER_TICK);
		std::cout << std::flush;

		std::vector<int> frequencies(512);

		if (interval >= 2.0)
		{
			/* Write the test pattern. */

			Fluxmap outFluxmap;
			while (outFluxmap.duration() < period)
			{
				outFluxmap.appendInterval(ticks);
				outFluxmap.appendPulse();
			}
			usbWrite(spec.locations[0].side, outFluxmap.rawBytes(), 0);

			/* Read the test pattern in again. */

			Fluxmap inFluxmap;
			inFluxmap.appendBytes(usbRead(spec.locations[0].side, true, period, 0));

			/* Compute histogram. */

			FluxmapReader fmr(inFluxmap);
			fmr.seek((double)period*0.1); /* skip first 10% and last 10% as contains junk */
			fmr.findEvent(F_BIT_PULSE);
			while (fmr.tell().ns() < ((double)period*0.9))
			{
				unsigned ticks = fmr.findEvent(F_BIT_PULSE);
				if (ticks < frequencies.size())
					frequencies[ticks]++;
			}
		}

		/* Compute standard deviation. */

		int sum = 0;
		int prod = 0;
		for (int i=0; i<frequencies.size(); i++)
		{
			sum += frequencies[i];
			prod += i * frequencies[i];
		}
		if (sum == 0)
			std::cout << "failed\n";
		else
		{
			double mean = prod / sum;
			double sqsum = 0;
			for (int i=0; i<frequencies.size(); i++)
			{
				double dx = (double)i - mean;
				sqsum += (double)frequencies[i] * dx * dx;
			}
			double stdv = sqrt(sqsum / sum);
			std::cout << fmt::format("{:.4f} {:.4f}\n", stdv, mean/TICKS_PER_US);

		}

		if (writeCsv.get() != "")
		{
			csv << interval;
			for (int i : frequencies)
				csv << "," << i;
			csv << '\n';
		}
	}

    return 0;
}

