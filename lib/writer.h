#ifndef WRITER_H
#define WRITER_H

class Fluxmap;

extern void setWriterDefaults(int minTrack, int maxTrack, int minSide, int maxSide);

extern void writeTracks(const std::function<Fluxmap(int track, int side)> producer);

extern void fillBitmapTo(std::vector<bool>& bitmap,
		unsigned& cursor, unsigned terminateAt,
		const std::vector<bool>& pattern);
	
#endif
