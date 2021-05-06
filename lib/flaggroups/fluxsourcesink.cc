#include "globals.h"
#include "flags.h"
#include "flaggroups/fluxsourcesink.h"

FlagGroup fluxSourceSinkFlags;

BoolFlag fluxSourceSinkFortyTrack(
	{ "--40-track" },
	"indicates a 40 track drive",
	false);


