#include "globals.h"
#include "flags.h"
#include "usb/usb.h"
#include "dataspec.h"
#include "fluxmap.h"
#include "decoders/fluxmapreader.h"
#include "writer.h"
#include "protocol.h"
#include "fmt/format.h"
#include "dep/agg/include/agg2d.h"
#include "dep/stb/stb_image_write.h"
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
	"");

static DataSpecFlag writeImg(
	{ "--write-img" },
	"Draw a graph of the response data",
	":w=640:h=480");

static agg::srgba8 hsvToRgb(double h, double s, double v)
{
	h = fmod(h, 360.0);
	double hh = h / 60.0;
	int i = (int)hh;
	double ff = hh - i;
	double p = v * (1.0 - s);
	double q = v * (1.0 - s*ff);
	double t = v * (1.0 - s*(1.0 - ff));

	double r, g, b;
    switch (i) {
		case 0: r = v; g = t; b = p; break;
		case 1: r = q; g = v; b = p; break;
		case 2: r = p; g = v; b = t; break;
		case 3: r = p; g = q; b = v; break;
		case 4: r = t; g = p; b = v; break;
		case 5:
		default: r = v; g = p; b = q; break;
    }

	return agg::srgba8(b*255.0, g*255.0, r*255.0, 255);
}

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

	int numRows = (maxInterval - minInterval) / intervalStep;
	const int numColumns = 512;
	double frequencies[numRows][numColumns] = {};

	int row = 0;
	for (double interval = minInterval; interval<maxInterval; interval += intervalStep, row++)
	{
		unsigned ticks = (unsigned) (interval * TICKS_PER_US);
		std::cout << fmt::format("Interval {:.2f}: ", ticks * US_PER_TICK);
		std::cout << std::flush;

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
				if (ticks < numColumns)
					frequencies[row][ticks]++;
			}
		}

		/* Compute mean and normalise. */

		double sum = 0.0;
		double prod = 0.0;
		double max = 0.0;
		for (int i=0; i<numColumns; i++)
		{
			sum += frequencies[row][i];
			prod += i * frequencies[row][i];
			max = std::max(max, frequencies[row][i]);
		}
		if (max != 0.0)
			for (int i=0; i<numColumns; i++)
				frequencies[row][i] /= max;

		if (sum == 0)
			std::cout << "failed\n";
		else
		{
			double mean = prod / sum;
			std::cout << fmt::format("{:.4f}\n", mean/TICKS_PER_US);

		}

		if (writeCsv.get() != "")
		{
			csv << interval;
			for (double d : frequencies[row])
				csv << "," << d;
			csv << '\n';
		}
	}

	BitmapSpec bitmapSpec(writeImg);
	if (!bitmapSpec.filename.empty())
	{
		Agg2D& painter = bitmapSpec.painter();
		painter.clearAll(0xdd, 0xdd, 0xdd);

		const double MARGIN = 20;
		agg::rect_d drawableBounds = {
			MARGIN, MARGIN,
			bitmapSpec.width - MARGIN, bitmapSpec.height - MARGIN
		};
		agg::rect_d colourbarBounds = {
			drawableBounds.x2 - MARGIN*4, drawableBounds.y1,
			drawableBounds.x2, drawableBounds.y2
		};
		agg::rect_d graphBounds = {
			drawableBounds.x1, drawableBounds.y1,
			colourbarBounds.x1, drawableBounds.y2
		};
		double blockWidth = (graphBounds.x2 - graphBounds.x1) / numColumns;
		double blockHeight = (graphBounds.y2 - graphBounds.y1) / numRows;

		painter.noLine();
		for (int x=0; x<numColumns; x++)
		{
			for (int y=0; y<numRows; y++)
			{
				double xx = graphBounds.x1 + x*blockWidth;
				double yy = graphBounds.y2 - y*blockHeight;

				painter.fillColor(hsvToRgb(360.0 * ((double)x/numColumns), 1.0, 1.0));
				painter.rectangle(xx, yy-blockHeight, xx+blockWidth, yy);
			}
		}

		painter.noFill();
		painter.lineColor(0, 0, 0);
		painter.rectangle(graphBounds.x1, drawableBounds.y1, drawableBounds.x2, drawableBounds.y2);
		painter.rectangle(colourbarBounds.x1, drawableBounds.y1, drawableBounds.x2, drawableBounds.y2);
		bitmapSpec.save();
	}

    return 0;
}

