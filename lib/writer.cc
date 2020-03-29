#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "writer.h"
#include "sql.h"
#include "protocol.h"
#include "usb.h"
#include "dataspec.h"
#include "encoders/encoders.h"
#include "fluxsource/fluxsource.h"
#include "fluxsink/fluxsink.h"
#include "fmt/format.h"
#include "record.h"
#include "image.h"
#include "sector.h"
#include "sectorset.h"

FlagGroup writerFlags { &hardwareFluxSourceFlags, &hardwareFluxSinkFlags };

static DataSpecFlag dest(
    { "--dest", "-d" },
    "destination for data",
    ":d=0:t=0-79:s=0-1");

static DataSpecFlag input(
    { "--input", "-i" },
    "input image file to read from",
    "");

static SettableFlag highDensityFlag(
	{ "--high-density", "-H" },
	"set the drive to high density mode");

static sqlite3* outdb;

void setWriterDefaultDest(const std::string& dest)
{
    ::dest.set(dest);
}

void setWriterDefaultInput(const std::string& input)
{
    ::input.set(input);
}

void writeTracks(
	const std::function<std::unique_ptr<Fluxmap>(int track, int side)> producer)
{
    const FluxSpec spec(dest);

    std::cout << "Writing to: " << dest << std::endl;

	setHardwareFluxSourceDensity(highDensityFlag);
	setHardwareFluxSinkDensity(highDensityFlag);

	std::shared_ptr<FluxSink> fluxSink = FluxSink::create(spec);

    for (const auto& location : spec.locations)
    {
        std::cout << fmt::format("{0:>3}.{1}: ", location.track, location.side) << std::flush;
        std::unique_ptr<Fluxmap> fluxmap = producer(location.track, location.side);
        if (!fluxmap)
        {
            /* Create an empty fluxmap for writing. */
            fluxmap.reset(new Fluxmap());
        }

        /* Precompensation actually seems to make things worse, so let's leave
            * it disabled for now. */
        //fluxmap->precompensate(PRECOMPENSATION_THRESHOLD_TICKS, 2);
        fluxSink->writeFlux(location.track, location.side, *fluxmap);
        std::cout << fmt::format(
            "{0} ms in {1} bytes", int(fluxmap->duration()/1e6), fluxmap->bytes()) << std::endl;
    }
}

void fillBitmapTo(std::vector<bool>& bitmap,
	unsigned& cursor, unsigned terminateAt,
	const std::vector<bool>& pattern)
{
	while (cursor < terminateAt)
	{
		for (bool b : pattern)
		{
			if (cursor < bitmap.size())
				bitmap[cursor++] = b;
		}
	}
}

void writeDiskCommand(AbstractEncoder& encoder)
{
    const ImageSpec spec(input);
	SectorSet allSectors = readSectorsFromFile(spec);
	writeTracks(
		[&](int track, int side) -> std::unique_ptr<Fluxmap>
		{
			return encoder.encode(track, side, allSectors);
		}
	);
}
