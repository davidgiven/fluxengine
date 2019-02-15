#ifndef SECTORSET_H
#define SECTORSET_H

class Sector;

class SectorSet
{
public:
	typedef std::tuple<int, int, int> key_t;

public:
	static key_t keyof(int track, int head, int sector)
	{ return std::tuple<int, int, int>(track, head, sector); }

	SectorSet() {};

	std::unique_ptr<Sector>& operator[](const key_t& key);
	Sector* operator[](const key_t& key) const;

	void calculateSize(int& numTracks, int& numHeads, int& numSectors,
		int& sectorSize) const;

private:
	std::map<const key_t, std::unique_ptr<Sector>> _data;
};

#endif

