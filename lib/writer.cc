#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "writer.h"
#include "sql.h"
#include "protocol.h"
#include "usb/usb.h"
#include "encoders/encoders.h"
#include "decoders/decoders.h"
#include "fluxsource/fluxsource.h"
#include "fluxsink/fluxsink.h"
#include "imagereader/imagereader.h"
#include "fmt/format.h"
#include "sector.h"
#include "image.h"
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

void writeTracksAndVerify(
	FluxSink& fluxSink,
	AbstractEncoder& encoder,
	FluxSource& fluxSource,
	AbstractDecoder& decoder,
	Image& image)
{
    std::cout << "Writing to: " << fluxSink << std::endl;

	for (unsigned cylinder : iterate(config.cylinders()))
	{
		for (unsigned head : iterate(config.heads()))
		{
			std::cout << fmt::format("{0:>3}.{1}: Write:  ", cylinder, head) << std::flush;
			std::unique_ptr<Fluxmap> fluxmap = encoder.encode(cylinder, head, image);
			if (!fluxmap)
			{
				/* Erase this track rather than writing. */

				fluxmap.reset(new Fluxmap());
				fluxSink.writeFlux(cylinder, head, *fluxmap);
				std::cout << "erased\n";
			}
			else
			{
				for (int retry = 0;; retry++)
				{
					/* Precompensation actually seems to make things worse, so let's leave
						* it disabled for now. */
					//fluxmap->precompensate(PRECOMPENSATION_THRESHOLD_TICKS, 2);
					fluxSink.writeFlux(cylinder, head, *fluxmap);
					std::cout << fmt::format(
						"{0} ms in {1} bytes\n", int(fluxmap->duration()/1e6), fluxmap->bytes());

					std::cout << fmt::format("       Verify: ", cylinder, head) << std::flush;
					std::shared_ptr<Fluxmap> writtenFluxmap = fluxSource.readFlux(cylinder, head);
						std::cout << fmt::format(
							"{0} ms in {1} bytes\n", int(writtenFluxmap->duration()/1e6), writtenFluxmap->bytes());
					const auto trackdata = decoder.decodeToSectors(writtenFluxmap, cylinder, head);

					std::vector<std::shared_ptr<Sector>> gotSectors = trackdata->sectors;
					std::remove_if(gotSectors.begin(), gotSectors.end(), [](const auto& s) { return s->status != Sector::OK; });
					std::sort(gotSectors.begin(), gotSectors.end(), sectorPointerSortPredicate);
					auto newLast = std::unique(gotSectors.begin(), gotSectors.end(), sectorPointerEqualsPredicate);
					gotSectors.erase(newLast, gotSectors.end());

					std::vector<std::shared_ptr<Sector>> wantedSectors = encoder.collectSectors(cylinder, head, image);
					std::sort(wantedSectors.begin(), wantedSectors.end(), sectorPointerSortPredicate);

					if (std::equal(gotSectors.begin(), gotSectors.end(), wantedSectors.begin(), wantedSectors.end(),
							sectorPointerEqualsPredicate))
						break;

					if (retry == config.decoder().retries())
						Error() << "Write failed; uncorrectable error during write.";

					std::cout << fmt::format("       Rewrite: ", cylinder, head) << std::flush;
				}
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

void writeDiskCommand(ImageReader& imageReader, AbstractEncoder& encoder, FluxSink& fluxSink,
		AbstractDecoder* decoder, FluxSource* fluxSource)
{
	Image image = imageReader.readImage();
	if (fluxSource && decoder)
		writeTracksAndVerify(fluxSink, encoder, *fluxSource, *decoder, image);
	else
		writeTracks(fluxSink,
			[&](int physicalTrack, int physicalSide) -> std::unique_ptr<Fluxmap>
			{
				return encoder.encode(physicalTrack, physicalSide, image);
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

