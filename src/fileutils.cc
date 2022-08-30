#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "sector.h"
#include "proto.h"
#include "readerwriter.h"
#include "lib/decoders/decoders.h"
#include "lib/encoders/encoders.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/fluxsink/fluxsink.h"
#include "lib/imagereader/imagereader.h"
#include "fmt/format.h"
#include "fluxengine.h"
#include "lib/vfs/sectorinterface.h"
#include "lib/vfs/vfs.h"
#include <google/protobuf/text_format.h>
#include <fstream>

FlagGroup fileFlags;

static StringFlag image({"-i", "--image"},
    "image to work on",
    "",
    [](const auto& value)
    {
        ImageReader::updateConfigForFilename(
            config.mutable_image_reader(), value);
    });

static StringFlag flux({"-f", "--flux"},
    "flux source to work on",
    "",
    [](const auto& value)
    {
		FluxSource::updateConfigForFilename(
            config.mutable_flux_source(), value);
		FluxSink::updateConfigForFilename(
            config.mutable_flux_sink(), value);
    });

std::unique_ptr<Filesystem> createFilesystemFromConfig()
{
    std::shared_ptr<SectorInterface> sectorInterface;
	if (config.has_flux_source())
	{
		std::shared_ptr<FluxSource> fluxSource(FluxSource::create(config.flux_source()));
		std::shared_ptr<FluxSink> fluxSink(FluxSink::create(config.flux_sink()));
		std::shared_ptr<AbstractEncoder> encoder(AbstractEncoder::create(config.encoder()));
		std::shared_ptr<AbstractDecoder> decoder(AbstractDecoder::create(config.decoder()));
		sectorInterface = SectorInterface::createFluxSectorInterface(fluxSource, fluxSink, encoder, decoder);
	}
	else
	{
		auto reader = ImageReader::create(config.image_reader());
		std::shared_ptr<Image> image(std::move(reader->readImage()));
		sectorInterface = SectorInterface::createImageSectorInterface(image);
	}

    return Filesystem::createFilesystem(config.filesystem(), sectorInterface);
}

