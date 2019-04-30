#ifndef SECTORSET_H
#define SECTORSET_H

class Sector;

class SectorSet
{
private:
	typedef std::tuple<int, int, int> key_t;

public:
	static key_t keyof(int track, int head, int sector)
	{ return std::tuple<int, int, int>(track, head, sector); }

	SectorSet() {};

	Sector*& get(int track, int head, int sector);
	Sector* get(int track, int head, int sector) const;

	void calculateSize(int& numTracks, int& numHeads, int& numSectors,
		int& sectorSize) const;

private:
	std::map<const key_t, Sector*> _data;
};

#endif

