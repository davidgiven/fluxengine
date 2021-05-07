#include "globals.h"
#include "flags.h"
#include "flaggroups/fluxsourcesink.h"

FlagGroup fluxSourceSinkFlags;

SettableFlag fluxSourceSinkFortyTrack(
	{ "--40-track", "-4" },
	"indicates a 40 track drive");

SettableFlag fluxSourceSinkHighDensity(
	{ "--high-density", "-H" },
	"set the drive to high density mode");


