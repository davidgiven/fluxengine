#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "writer.h"
#include "sql.h"
#include "protocol.h"
#include "usb/usb.h"
#include "encoders/encoders.h"
#include "fluxsource/fluxsource.h"
#include "fluxsink/fluxsink.h"
#include "imagereader/imagereader.h"
#include "fmt/format.h"
#include "record.h"
#include "sector.h"
#include "sectorset.h"
#include "lib/config.pb.h"
#include "proto.h"

static sqlite3* outdb;

void writeTracks(
	FluxSink& fluxSink,
	const std::function<std::unique_ptr<Fluxmap>(int track, int side)> producer)
{
    std::cout << "Writing to: " << fluxSink << std::endl;

	for (unsigned cylinder : iterate(config.cylinders()))
	{
		for (unsigned head : iterate(config.heads()))
		{
			std::cout << fmt::format("{0:>3}.{1}: ", cylinder, head) << std::flush;
			std::unique_ptr<Fluxmap> fluxmap = producer(cylinder, head);
			if (!fluxmap)
			{
				/* Erase this track rather than writing. */

				fluxmap.reset(new Fluxmap());
				fluxSink.writeFlux(cylinder, head, *fluxmap);
				std::cout << "erased\n";
			}
			else
			{
				/* Precompensation actually seems to make things worse, so let's leave
					* it disabled for now. */
				//fluxmap->precompensate(PRECOMPENSATION_THRESHOLD_TICKS, 2);
				fluxSink.writeFlux(cylinder, head, *fluxmap);
				std::cout << fmt::format(
					"{0} ms in {1} bytes", int(fluxmap->duration()/1e6), fluxmap->bytes()) << std::endl;
			}
		}
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

void writeDiskCommand(ImageReader& imageReader, AbstractEncoder& encoder, FluxSink& fluxSink)
{
	SectorSet allSectors = imageReader.readImage();
	writeTracks(fluxSink,
		[&](int track, int side) -> std::unique_ptr<Fluxmap>
		{
			return encoder.encode(track, side, allSectors);
		}
	);
}

void writeRawDiskCommand(FluxSource& fluxSource, FluxSink& fluxSink)
{
	writeTracks(fluxSink,
		[&](int track, int side) -> std::unique_ptr<Fluxmap>
		{
			return fluxSource.readFlux(track, side);
		}
	);
}

