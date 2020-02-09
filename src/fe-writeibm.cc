#include "globals.h"
#include "flags.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "ibm/ibm.h"
#include "writer.h"
#include "fmt/format.h"
#include "image.h"
#include <fstream>

static FlagGroup flags { &writerFlags };

static IntFlag sectorsPerTrack(
	{ "--ibm-sectors-per-track" },
	"Number of sectors per track to write.",
	0);

static IntFlag sectorSize(
	{ "--ibm-sector-size" },
	"Size of the sectors to write (bytes).",
	0);

static BoolFlag emitIam(
	{ "--ibm-emit-iam" },
	"Whether to emit an IAM record at the start of the track.",
	false);

static IntFlag startSectorId(
	{ "--ibm-start-sector-id" },
	"Sector ID of first sector.",
	1);

static IntFlag clockSpeedKhz(
	{ "--ibm-clock-rate-khz" },
	"Clock rate of data to write.",
	0);

static BoolFlag useFm(
	{ "--ibm-use-fm" },
	"Write in FM mode, rather than MFM.",
	false);

static IntFlag gap1(
	{ "--ibm-gap1-bytes" },
	"Size of gap 1.",
	0);

static IntFlag gap3(
	{ "--ibm-gap3-bytes" },
	"Size of gap 3.",
	0);

static IntFlag damByte(
	{ "--ibm-dam-byte" },
	"Value of DAM byte to emit.",
	0xf8);

static StringFlag sectorSkew(
	{ "--ibm-sector-skew" },
	"Order to emit sectors.",
	"");

static ActionFlag preset1440(
	{ "--ibm-preset-1440" },
	"Preset parameters to a 3.5\" 1440kB disk.",
	[] {
		sectorsPerTrack.setDefaultValue(18);
		sectorSize.setDefaultValue(512);
		emitIam.setDefaultValue(true);
		clockSpeedKhz.setDefaultValue(500);
		gap1.setDefaultValue(0x1b);
		gap3.setDefaultValue(0x6c);
		sectorSkew.setDefaultValue("123456789abcdefghi");
	});

int mainWriteIbm(int argc, const char* argv[])
{
    setWriterDefaultInput(":c=80:h=2:s=18:b=512");
	setWriterDefaultDest(":d=0:t=0-79:s=2");
    flags.parseFlags(argc, argv);

	IbmParameters parameters;
	parameters.sectorsPerTrack = sectorsPerTrack;
	parameters.sectorSize = sectorSize;
	parameters.emitIam = emitIam;
	parameters.clockSpeedKhz = clockSpeedKhz;
	parameters.useFm = useFm;
	parameters.gap1 = gap1;
	parameters.gap3 = gap3;
	parameters.damByte = (uint8_t) damByte;

	IbmEncoder encoder(parameters);
	writeDiskCommand(encoder);

    return 0;
}

