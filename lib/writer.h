#ifndef WRITER_H
#define WRITER_H

#include "flags.h"

extern FlagGroup writerFlags;

class Fluxmap;
class AbstractEncoder;
class Geometry;

extern void setWriterDefaultDest(const std::string& dest);
extern void setWriterDefaultInput(const std::string& input);

extern void writeTracks(const std::function<std::unique_ptr<Fluxmap>(int track, int side)> producer);

extern void fillBitmapTo(std::vector<bool>& bitmap,
		unsigned& cursor, unsigned terminateAt,
		const std::vector<bool>& pattern);
	
extern void writeDiskCommand(AbstractEncoder& encoder);

#endif
