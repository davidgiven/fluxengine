#ifndef WRITER_H
#define WRITER_H

class Fluxmap;

extern void writeTrack(int track, int side, const Fluxmap& fluxmap);

extern void fillBitmapTo(std::vector<bool>& bitmap,
		unsigned& cursor, unsigned terminateAt,
		const std::vector<bool>& pattern);
	
#endif
