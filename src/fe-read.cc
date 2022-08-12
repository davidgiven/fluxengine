#include "globals.h"
#include "flags.h"
#include "readerwriter.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "macintosh/macintosh.h"
#include "sector.h"
#include "proto.h"
#include "fluxsource/fluxsource.h"
#include "fluxsink/fluxsink.h"
#include "arch/brother/brother.h"
#include "arch/ibm/ibm.h"
#include "imagewriter/imagewriter.h"
#include "fmt/format.h"
#include "fluxengine.h"
#include <google/protobuf/text_format.h>
#include <fstream>

static FlagGroup flags;

static StringFlag sourceFlux(
	{ "-s", "--source" },
	"flux file to read from",
	"",
	[](const auto& value)
	{
		FluxSource::updateConfigForFilename(config.mutable_flux_source(), value);
	});

static StringFlag destImage(
	{ "-o", "--output" },
	"destination image to write",
	"",
	[](const auto& value)
	{
		ImageWriter::updateConfigForFilename(config.mutable_image_writer(), value);
	});

static StringFlag copyFluxTo(
	{ "--copy-flux-to" },
	"while reading, copy the read flux to this file",
	"",
	[](const auto& value)
	{
		FluxSink::updateConfigForFilename(config.mutable_decoder()->mutable_copy_flux_to(), value);
	});

static StringFlag solvedFlux(
	{ "-r", "--solved" },
	"after reading, write a reconstructed/solved fluxmap to this file",
	"",
	[](const auto& value)
	{
		FluxSink::updateConfigForFilename(config.mutable_solved_flux(), value);
	});

static StringFlag srcTracks(
	{ "--cylinders", "-c" },
	"tracks to read from",
	"",
	[](const auto& value)
	{
		setRange(config.mutable_tracks(), value);
	});

static StringFlag srcHeads(
	{ "--heads", "-h" },
	"heads to read from",
	"",
	[](const auto& value)
	{
		setRange(config.mutable_heads(), value);
	});

int mainRead(int argc, const char* argv[])
{
	if (argc == 1)
		showProfiles("read", formats);
	config.mutable_flux_source()->mutable_drive();
    flags.parseFlagsWithConfigFiles(argc, argv, formats);

	if (config.decoder().copy_flux_to().has_drive())
		Error() << "you cannot copy flux to a hardware device";

	std::unique_ptr<FluxSource> fluxSource(FluxSource::create(config.flux_source()));
	std::unique_ptr<AbstractDecoder> decoder(AbstractDecoder::create(config.decoder()));
	std::unique_ptr<ImageWriter> writer(ImageWriter::create(config.image_writer()));
	std::unique_ptr<FluxSink> solvedFlux;
	std::unique_ptr<AbstractEncoder> solvedEncoder;
	if (config.has_solved_flux()) {
		solvedFlux = FluxSink::create(config.solved_flux());	}

	readDiskCommand(*fluxSource, *decoder, *writer, solvedFlux.get());

    return 0;
}

