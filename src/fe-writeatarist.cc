#include "globals.h"
#include "flags.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "ibm/ibm.h"
#include "writer.h"
#include "fmt/format.h"
#include <fstream>

static FlagGroup flags { &writerFlags };

static IntFlag sectorSize(
	{ "--st-sector-size" },
	"Size of the sectors to write (bytes).",
	512);
static IntFlag gap1(
	{ "--st-gap1-bytes" },
	"Size of gap 1 (the post-index gap).",
	60);

static IntFlag gap2(
	{ "--st-gap2-bytes" },
	"Size of gap 2 (the post-ID gap).",
	22);

static IntFlag gap3(
	{ "--st-gap3-bytes" },
	"Size of gap 3 (the post-data or format gap).",
	40);

static StringFlag sectorSkew(
	{ "--st-sector-skew" },
	"Order to emit sectors.",
	"");

static BoolFlag swapSides(
	{ "--ibm-swap-sides" },
	"Swap sides while writing.",
	false);

static ActionFlag preset360(
	{ "--st-preset-360" },
	"Preset parameters to a 3.5\" 360kB disk (1 side, 80 tracks, 9 sectors).",
	[] {
		setWriterDefaultDest(":d=0:s=0:t=0-79");
		setWriterDefaultInput(":c=80:h=1:s=9:b=512");
		sectorSkew.setDefaultValue("012345678");
	});

static ActionFlag preset370(
	{ "--st-preset-380" },
	"Preset parameters to a 3.5\" 370kB disk (1 side, 82 tracks, 9 sectors).",
	[] {
		setWriterDefaultDest(":d=0:s=0:t=0-81");
		setWriterDefaultInput(":c=82:h=1:s=9:b=512");
		sectorSkew.setDefaultValue("012345678");
	});

static ActionFlag preset400(
	{ "--st-preset-400" },
	"Preset parameters to a 3.5\" 400kB disk (1 side, 80 Tracks, 10 sectors).",
	[] {
		setWriterDefaultDest(":d=0:s=0:t=0-79");
		setWriterDefaultInput(":c=80:h=1:s=10:b=512");
		sectorSkew.setDefaultValue("0123456789");
	});

static ActionFlag preset410(
	{ "--st-preset-410" },
	"Preset parameters to a 3.5\" 410kB disk (1 side, 82 tracks, 10 sectors).",
	[] {
		setWriterDefaultDest(":d=0:s=0:t=0-81");
		setWriterDefaultInput(":c=82:h=1:s=10:b=512");
		sectorSkew.setDefaultValue("0123456789");
	});

static ActionFlag preset720(
	{ "--st-preset-720" },
	"Preset parameters to a 3.5\" 720kB disk (2 sides, 80 tracks, 9 sectors).",
	[] {
		setWriterDefaultDest(":d=0:s=0-1:t=0-79");
		setWriterDefaultInput(":c=80:h=2:s=9:b=512");
		sectorSkew.setDefaultValue("012345678");
	});

static ActionFlag preset740(
	{ "--st-preset-740" },
	"Preset parameters to a 3.5\" 740kB disk (2 sides, 82 tracks, 9 sectors).",
	[] {
		setWriterDefaultDest(":d=0:s=0-1:t=0-81");
		setWriterDefaultInput(":c=82:h=2:s=9:b=512");
		sectorSkew.setDefaultValue("012345678");
	});

static ActionFlag preset800(
	{ "--st-preset-800" },
	"Preset parameters to a 3.5\" 800kB disk (2 sides, 80 tracks, 10 sectors).",
	[] {
		setWriterDefaultDest(":d=0:s=0-1:t=0-79");
		setWriterDefaultInput(":c=80:h=2:s=10:b=512");
		sectorSkew.setDefaultValue("0123456789");
	});

static ActionFlag preset820(
	{ "--st-preset-820" },
	"Preset parameters to a 3.5\" 820kB disk (2 sides, 82 tracks, 10 sectors).",
	[] {
		setWriterDefaultDest(":d=0:s=0-1:t=0-81");
		setWriterDefaultInput(":c=82:h=2:s=10:b=512");
		sectorSkew.setDefaultValue("0123456789");
	});

int mainWriteAtariST(int argc, const char* argv[])
{
	setWriterDefaultDest(":d=0:t=0-79:s=0-1");
	flags.parseFlags(argc, argv);

	IbmParameters parameters;
	parameters.trackLengthMs = 200;
	parameters.sectorSize = sectorSize;
	parameters.emitIam = false;
	parameters.startSectorId = 1;
	parameters.clockRateKhz = 250;
 	parameters.useFm = false;
	parameters.idamByte = 0x5554;
	parameters.damByte = 0x5545;
	parameters.gap0 = 0;
	parameters.gap1 = gap1;
	parameters.gap2 = gap2;
	parameters.gap3 = gap3;
	parameters.sectorSkew = sectorSkew;
	parameters.swapSides = swapSides;

	IbmEncoder encoder(parameters);
	writeDiskCommand(encoder);

    return 0;
}
