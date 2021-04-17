#include "globals.h"
#include "flags.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "ibm/ibm.h"
#include "writer.h"
#include "fmt/format.h"
#include <fstream>

static FlagGroup flags { &writerFlags };

static IntFlag trackLengthMs(
	{ "--ibm-track-length-ms" },
	"Length of a track in milliseconds.",
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

static IntFlag clockRateKhz(
	{ "--ibm-clock-rate-khz" },
	"Clock rate of data to write.",
	0);

static BoolFlag useFm(
	{ "--ibm-use-fm" },
	"Write in FM mode, rather than MFM.",
	false);

static HexIntFlag idamByte(
	{ "--ibm-idam-byte" },
	"16-bit RAW bit pattern to use for the IDAM ID byte",
	0);

static HexIntFlag damByte(
	{ "--ibm-dam-byte" },
	"16-bit RAW bit pattern to use for the DAM ID byte",
	0);

static IntFlag gap0(
	{ "--ibm-gap0-bytes" },
	"Size of gap 0 (the pre-index gap)",
	0);

static IntFlag gap1(
	{ "--ibm-gap1-bytes" },
	"Size of gap 1 (the post-index gap).",
	0);

static IntFlag gap2(
	{ "--ibm-gap2-bytes" },
	"Size of gap 2 (the post-ID gap).",
	0);

static IntFlag gap3(
	{ "--ibm-gap3-bytes" },
	"Size of gap 3 (the post-data or format gap).",
	0);

static StringFlag sectorSkew(
	{ "--ibm-sector-skew" },
	"Order to emit sectors.",
	"");

static BoolFlag swapSides(
	{ "--ibm-swap-sides" },
	"Swap sides while writing. Needed for Commodore 1581, CMD FD-2000, Thomson TO7.",
	false);

/* --- IBM disks ----------------------------------------------------------- */

static void set_ibm_defaults()
{
	sectorSize.setDefaultValue(512);
	emitIam.setDefaultValue(true);
	idamByte.setDefaultValue(0x5554);
	damByte.setDefaultValue(0x5545);
	gap0.setDefaultValue(80);
	gap1.setDefaultValue(50);
	gap2.setDefaultValue(22);
	gap3.setDefaultValue(80);
	swapSides.setDefaultValue(false);
}

static ActionFlag preset1440(
	{ "--ibm-preset-1440" },
	"Preset parameters to a PC 3.5\" 1440kB disk.",
	[] {
		setWriterDefaultInput(":c=80:h=2:s=18:b=512");
		trackLengthMs.setDefaultValue(200);
		clockRateKhz.setDefaultValue(500);
		sectorSkew.setDefaultValue("0123456789abcdefgh");
		set_ibm_defaults();
	});

static ActionFlag preset720(
	{ "--ibm-preset-720" },
	"Preset parameters to a PC 3.5\" 720kB disk.",
	[] {
		setWriterDefaultInput(":c=80:h=2:s=9:b=512");
		trackLengthMs.setDefaultValue(200);
		clockRateKhz.setDefaultValue(250);
		sectorSkew.setDefaultValue("012345678");
	});

/* --- Commodore disks ----------------------------------------------------- */

static ActionFlag presetCBM1581(
	{ "--ibm-preset-commodore-1581" },
	"Preset parameters to a Commodore 3.5\" 800kB 1581 disk.",
	[] {
		setWriterDefaultInput(":c=80:h=2:s=10:b=512");
		trackLengthMs.setDefaultValue(200);
		sectorSize.setDefaultValue(512);
		startSectorId.setDefaultValue(1);
		emitIam.setDefaultValue(false);
		clockRateKhz.setDefaultValue(250);
		idamByte.setDefaultValue(0x5554);
		damByte.setDefaultValue(0x5545);
		gap0.setDefaultValue(80);
		gap1.setDefaultValue(80); //as emitIam is false this value remains unused
		gap2.setDefaultValue(22);
		gap3.setDefaultValue(34);
		sectorSkew.setDefaultValue("0123456789");
		swapSides.setDefaultValue(true);
	});
/* --- HP-LIF ----------------------------------------------------- */

static ActionFlag presetHPLIF(
	{ "--ibm-preset-hp-lif" },
	"Preset parameters to a HP-LIF 3.5\" 770kB HP disk.",
	[] {
		setWriterDefaultDest(":d=0:s=0-1:t=0-76");
		setWriterDefaultInput(":c=77:h=2:s=5:b=1024");
		trackLengthMs.setDefaultValue(200);
		sectorSize.setDefaultValue(1024);
		startSectorId.setDefaultValue(1);
		emitIam.setDefaultValue(true);
		clockRateKhz.setDefaultValue(250);
		idamByte.setDefaultValue(0x5554);
		damByte.setDefaultValue(0x5545);
		gap0.setDefaultValue(80);
		gap1.setDefaultValue(50); //as emitIam is false this value remains unused
		gap2.setDefaultValue(22);
		gap3.setDefaultValue(44);
		sectorSkew.setDefaultValue("01234");
		swapSides.setDefaultValue(false);
	});

/* --- Atari ST disks ------------------------------------------------------ */

static void set_atari_defaults()
{
		trackLengthMs.setDefaultValue(200);
		sectorSize.setDefaultValue(512);
		startSectorId.setDefaultValue(1);
		emitIam.setDefaultValue(false);
		clockRateKhz.setDefaultValue(250);
		idamByte.setDefaultValue(0x5554);
		damByte.setDefaultValue(0x5545);
		gap0.setDefaultValue(80);
		gap1.setDefaultValue(80); //as emitIam is false this value remains unused
		gap2.setDefaultValue(22);
		gap3.setDefaultValue(34);
		swapSides.setDefaultValue(true);
}

static ActionFlag presetAtariST360(
	{ "--ibm-preset-atarist-360" },
	"Preset parameters to an Atari ST 3.5\" 360kB disk.",
	[] {
		setWriterDefaultDest(":d=0:s=0:t=0-79");
		setWriterDefaultInput(":c=80:h=1:s=9:b=512");
		sectorSkew.setDefaultValue("012345678");
		set_atari_defaults();
	});

static ActionFlag presetAtariST370(
	{ "--ibm-preset-atarist-380" },
	"Preset parameters to an Atari ST 3.5\" 370kB disk.",
	[] {
		setWriterDefaultDest(":d=0:s=0:t=0-81");
		setWriterDefaultInput(":c=82:h=1:s=9:b=512");
		sectorSkew.setDefaultValue("012345678");
		set_atari_defaults();
	});

static ActionFlag presetAtariST400(
	{ "--ibm-preset-atarist-400" },
	"Preset parameters to an Atari ST 3.5\" 400kB disk.",
	[] {
		setWriterDefaultDest(":d=0:s=0:t=0-79");
		setWriterDefaultInput(":c=80:h=1:s=10:b=512");
		sectorSkew.setDefaultValue("0123456789");
		set_atari_defaults();
	});

static ActionFlag presetAtariST410(
	{ "--ibm-preset-atarist-410" },
	"Preset parameters to an Atari ST 3.5\" 410kB disk.",
	[] {
		setWriterDefaultDest(":d=0:s=0:t=0-81");
		setWriterDefaultInput(":c=82:h=1:s=10:b=512");
		sectorSkew.setDefaultValue("0123456789");
		set_atari_defaults();
	});

static ActionFlag presetAtariST720(
	{ "--ibm-preset-atarist-720" },
	"Preset parameters to an Atari ST 3.5\" 720kB disk.",
	[] {
		setWriterDefaultDest(":d=0:s=0-1:t=0-79");
		setWriterDefaultInput(":c=80:h=2:s=9:b=512");
		sectorSkew.setDefaultValue("012345678");
		set_atari_defaults();
	});

static ActionFlag presetAtariST740(
	{ "--ibm-preset-atarist-740" },
	"Preset parameters to an Atari ST 3.5\" 740kB disk.",
	[] {
		setWriterDefaultDest(":d=0:s=0-1:t=0-81");
		setWriterDefaultInput(":c=82:h=2:s=9:b=512");
		sectorSkew.setDefaultValue("012345678");
		set_atari_defaults();
	});

static ActionFlag presetAtariST800(
	{ "--ibm-preset-atarist-800" },
	"Preset parameters to an Atari ST 3.5\" 800kB disk.",
	[] {
		setWriterDefaultDest(":d=0:s=0-1:t=0-79");
		setWriterDefaultInput(":c=80:h=2:s=10:b=512");
		sectorSkew.setDefaultValue("0123456789");
		set_atari_defaults();
	});

static ActionFlag presetAtariST820(
	{ "--ibm-preset-atarist-820" },
	"Preset parameters to an Atari ST 3.5\" 820kB disk.",
	[] {
		setWriterDefaultDest(":d=0:s=0-1:t=0-81");
		setWriterDefaultInput(":c=82:h=2:s=10:b=512");
		sectorSkew.setDefaultValue("0123456789");
		set_atari_defaults();
	});

int mainWriteIbm(int argc, const char* argv[])
{
	setWriterDefaultDest(":d=0:t=0-79:s=0-1");
    flags.parseFlags(argc, argv);

	IbmParameters parameters;
	parameters.trackLengthMs = trackLengthMs;
	parameters.sectorSize = sectorSize;
	parameters.emitIam = emitIam;
	parameters.startSectorId = startSectorId;
	parameters.clockRateKhz = clockRateKhz;
	parameters.useFm = useFm;
	parameters.idamByte = idamByte;
	parameters.damByte = damByte;
	parameters.gap0 = gap0;
	parameters.gap1 = gap1;
	parameters.gap2 = gap2;
	parameters.gap3 = gap3;
	parameters.sectorSkew = sectorSkew;
	parameters.swapSides = swapSides;

	IbmEncoder encoder(parameters);
	writeDiskCommand(encoder);

    return 0;
}
