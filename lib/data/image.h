#ifndef IMAGE_H
#define IMAGE_H

class Sector;

struct Geometry
{
    unsigned numTracks = 0;
    unsigned numSides = 0;
    unsigned firstSector = UINT_MAX;
    unsigned numSectors = 0;
    unsigned sectorSize = 0;
    bool irregular = false;
};

class Image
{
private:
    typedef std::tuple<unsigned, unsigned, unsigned> key_t;

public:
    Image();
    Image(std::set<std::shared_ptr<const Sector>>& sectors);

public:
    class const_iterator
    {
        typedef std::map<key_t, std::shared_ptr<const Sector>>::const_iterator
            wrapped_iterator_t;

    public:
        const_iterator(const wrapped_iterator_t& it): _it(it) {}
        std::shared_ptr<const Sector> operator*()
        {
            return _it->second;
        }

        void operator++()
        {
            _it++;
        }

        bool operator==(const const_iterator& other) const
        {
            return _it == other._it;
        }

        bool operator!=(const const_iterator& other) const
        {
            return _it != other._it;
        }

    private:
        wrapped_iterator_t _it;
    };

public:
    void calculateSize();

    void clear();
    void createBlankImage();
    bool empty() const;
    bool contains(unsigned track, unsigned side, unsigned sectorId) const;
    std::shared_ptr<const Sector> get(
        unsigned track, unsigned side, unsigned sectorId) const;
    std::shared_ptr<Sector> put(
        unsigned track, unsigned side, unsigned sectorId);
    void erase(unsigned track, unsigned side, unsigned sectorId);

    std::set<std::pair<unsigned, unsigned>> tracks() const;

    const_iterator begin() const
    {
        return const_iterator(_sectors.cbegin());
    }
    const_iterator end() const
    {
        return const_iterator(_sectors.cend());
    }

    void setGeometry(Geometry geometry)
    {
        _geometry = geometry;
    }
    const Geometry& getGeometry() const
    {
        return _geometry;
    }

private:
    Geometry _geometry = {0, 0, 0};
    std::map<key_t, std::shared_ptr<const Sector>> _sectors;
};

#endif
