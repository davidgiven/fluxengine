#ifndef IMAGE_H
#define IMAGE_H

/* 
 * Note that sectors here used zero-based numbering throughout (to make the
 * maths easier); traditionally floppy disk use 0-based track numbering and
 * 1-based sector numbering, which makes no sense.
 */
class Sector
{
public:
	enum
	{
		OK,
		BAD_CHECKSUM
	};

    Sector(int status, int track, int side, int sector, const std::vector<uint8_t>& data):
		status(status),
        track(track),
        side(side),
        sector(sector),
        data(data)
    {}

	const int status;
    const int track;
    const int side;
    const int sector;
    const std::vector<uint8_t> data;
};

extern void writeSectorsToFile(const std::vector<std::unique_ptr<Sector>>& sectors, const std::string& filename);

#endif
