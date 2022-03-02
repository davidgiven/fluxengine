#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "writer.h"
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
#include "logger.h"
#include "utils.h"
#include "lib/config.pb.h"
#include "proto.h"

void writeTracks(FluxSink& fluxSink,
    const std::function<std::unique_ptr<Fluxmap>(int track, int side)> producer)
{
    for (unsigned cylinder : iterate(config.cylinders()))
    {
        for (unsigned head : iterate(config.heads()))
        {
            testForEmergencyStop();
            Logger() << BeginWriteOperationLogMessage{cylinder, head};

            std::unique_ptr<Fluxmap> fluxmap = producer(cylinder, head);
            if (!fluxmap)
            {
                /* Erase this track rather than writing. */

                fluxmap.reset(new Fluxmap());
                fluxSink.writeFlux(cylinder, head, *fluxmap);
                Logger() << "erased";
            }
            else
            {
                fluxmap->rescale(config.flux_sink().rescale());
                /* Precompensation actually seems to make things worse, so let's
                 * leave it disabled for now. */
                // fluxmap->precompensate(PRECOMPENSATION_THRESHOLD_TICKS, 2);
                fluxSink.writeFlux(cylinder, head, *fluxmap);
                Logger() << fmt::format("{0} ms in {1} bytes",
                    int(fluxmap->duration() / 1e6),
                    fluxmap->bytes());
            }
            Logger() << EndWriteOperationLogMessage();
        }
    }
}

void writeTracksAndVerify(FluxSink& fluxSink,
    AbstractEncoder& encoder,
    FluxSource& fluxSource,
    AbstractDecoder& decoder,
    const Image& image)
{
    Logger() << fmt::format("Writing to: {}", (std::string)fluxSink);

    for (unsigned cylinder : iterate(config.cylinders()))
    {
        for (unsigned head : iterate(config.heads()))
        {
            testForEmergencyStop();

            auto sectors = encoder.collectSectors(cylinder, head, image);
            std::unique_ptr<Fluxmap> fluxmap =
                encoder.encode(cylinder, head, sectors, image);
            if (!fluxmap)
            {
                /* Erase this track rather than writing. */

                Logger() << BeginWriteOperationLogMessage{cylinder, head};
                fluxmap.reset(new Fluxmap());
                fluxSink.writeFlux(cylinder, head, *fluxmap);
                Logger() << EndWriteOperationLogMessage()
                         << fmt::format("erased");
            }
            else
            {
                fluxmap->rescale(config.flux_sink().rescale());
                std::sort(
                    sectors.begin(), sectors.end(), sectorPointerSortPredicate);

                for (int retry = 0;; retry++)
                {
                    /* Precompensation actually seems to make things worse, so
                     * let's leave it disabled for now. */
                    // fluxmap->precompensate(PRECOMPENSATION_THRESHOLD_TICKS,
                    // 2);
                    Logger() << BeginWriteOperationLogMessage{cylinder, head};
                    fluxSink.writeFlux(cylinder, head, *fluxmap);
                    Logger() << EndWriteOperationLogMessage()
                             << fmt::format("writing {0} ms in {1} bytes",
                                    int(fluxmap->duration() / 1e6),
                                    fluxmap->bytes());

                    Logger() << BeginReadOperationLogMessage{cylinder, head};
                    std::shared_ptr<Fluxmap> writtenFluxmap =
                        fluxSource.readFlux(cylinder, head);
                    Logger() << EndReadOperationLogMessage()
                             << fmt::format("verifying {0} ms in {1} bytes",
                                    int(writtenFluxmap->duration() / 1e6),
                                    writtenFluxmap->bytes());

                    const auto trackdata =
                        decoder.decodeToSectors(writtenFluxmap, cylinder, head);

                    std::vector<std::shared_ptr<const Sector>> gotSectors(
                        trackdata->sectors.begin(), trackdata->sectors.end());
                    gotSectors.erase(std::remove_if(gotSectors.begin(),
                                         gotSectors.end(),
                                         [](const auto& s)
                                         {
                                             return s->status != Sector::OK;
                                         }),
                        gotSectors.end());
                    std::sort(gotSectors.begin(),
                        gotSectors.end(),
                        sectorPointerSortPredicate);
                    gotSectors.erase(std::unique(gotSectors.begin(),
                                         gotSectors.end(),
                                         sectorPointerEqualsPredicate),
                        gotSectors.end());

                    if (std::equal(gotSectors.begin(),
                            gotSectors.end(),
                            sectors.begin(),
                            sectors.end(),
                            sectorPointerEqualsPredicate))
                        break;

                    if (retry == config.decoder().retries())
                        Error() << "Write failed; uncorrectable error during "
                                   "write.";

                    Logger() << "retrying";
                }
            }
        }
    }
}

void fillBitmapTo(std::vector<bool>& bitmap,
    unsigned& cursor,
    unsigned terminateAt,
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

void writeDiskCommand(const Image& image,
    AbstractEncoder& encoder,
    FluxSink& fluxSink,
    AbstractDecoder* decoder,
    FluxSource* fluxSource)
{
    if (fluxSource && decoder)
        writeTracksAndVerify(fluxSink, encoder, *fluxSource, *decoder, image);
    else
        writeTracks(fluxSink,
            [&](int physicalTrack, int physicalSide) -> std::unique_ptr<Fluxmap>
            {
                const auto& sectors =
                    encoder.collectSectors(physicalTrack, physicalSide, image);
				if (sectors.empty())
					return std::make_unique<Fluxmap>();
                return encoder.encode(
                    physicalTrack, physicalSide, sectors, image);
            });
}

void writeRawDiskCommand(FluxSource& fluxSource, FluxSink& fluxSink)
{
    writeTracks(fluxSink,
        [&](int track, int side) -> std::unique_ptr<Fluxmap>
        {
            return fluxSource.readFlux(track, side);
        });
}
