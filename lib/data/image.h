#ifndef IMAGE_H
#define IMAGE_H

#include "lib/data/locations.h"

class Sector;
class DiskLayout;

struct Geometry
{
    unsigned numCylinders = 0;
    unsigned numHeads = 0;
    unsigned firstSector = UINT_MAX;
    unsigned numSectors = 0;
    unsigned sectorSize = 0;
    bool irregular = false;
    unsigned totalBytes = 0;
};

class Image
{
public:
    Image();
    Image(const std::vector<std::shared_ptr<const Sector>>& sectors);

public:
    class const_iterator
    {
        typedef std::map<LogicalLocation,
            std::shared_ptr<const Sector>>::const_iterator wrapped_iterator_t;

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

    bool contains(const LogicalLocation& location) const;
    std::shared_ptr<const Sector> get(const LogicalLocation& location) const;
    std::shared_ptr<Sector> put(const LogicalLocation& location);
    void erase(const LogicalLocation& location);

    bool contains(const CylinderHead& ch, unsigned sector) const
    {
        return contains({ch.cylinder, ch.head, sector});
    }

    bool contains(unsigned cylinder, unsigned head, unsigned sector) const
    {
        return contains({cylinder, head, sector});
    }

    std::shared_ptr<const Sector> get(
        const CylinderHead& ch, unsigned sector) const
    {
        return get({ch.cylinder, ch.head, sector});
    }

    std::shared_ptr<const Sector> get(
        unsigned cylinder, unsigned head, unsigned sector) const
    {
        return get({cylinder, head, sector});
    }

    std::shared_ptr<Sector> put(const CylinderHead& ch, unsigned sector)
    {
        return put({ch.cylinder, ch.head, sector});
    }

    std::shared_ptr<Sector> put(
        unsigned cylinder, unsigned head, unsigned sector)
    {
        return put({cylinder, head, sector});
    }

    void erase(unsigned cylinder, unsigned head, unsigned sector)
    {
        erase({cylinder, head, sector});
    }

    void addMissingSectors(const DiskLayout& layout);

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
    std::map<LogicalLocation, std::shared_ptr<const Sector>> _sectors;
};

#endif
