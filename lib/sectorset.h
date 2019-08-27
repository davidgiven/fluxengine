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

	std::unique_ptr<Sector>& get(int track, int head, int sector);
	Sector* get(int track, int head, int sector) const;

	const std::map<const key_t, std::unique_ptr<Sector>>& get() const
	{ return _data; }

	void calculateSize(
		unsigned& numTracks, unsigned& numHeads, unsigned& numSectors,
		unsigned& sectorSize) const;

private:
	std::map<const key_t, std::unique_ptr<Sector>> _data;
};

#endif

