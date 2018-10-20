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
    Sector(int track, int side, int sector, const std::vector<uint8_t>& data):
        _track(track),
        _side(side),
        _sector(sector),
        _data(data)
    {}

    int track() const { return _track; }
    int side() const { return _side; }
    int sector() const { return _sector; }
    const std::vector<uint8_t>& data() const { return _data; }

private:
    const int _track;
    const int _side;
    const int _sector;
    const std::vector<uint8_t> _data;
};

extern void writeSectorsToFile(const std::vector<std::unique_ptr<Sector>>& sectors, const std::string& filename);

#endif
