#ifndef RAWBITS_H
#define RAWBITS_H

class RawBits
{
public:
    RawBits(std::unique_ptr<std::vector<bool>> bits,
        std::unique_ptr<std::vector<size_t>> indices):
        _bits(std::move(bits)),
        _indices(std::move(indices))
    {
    }

    typedef std::vector<bool>::const_iterator const_iterator;

    const_iterator begin() const
    {
        return _bits->begin();
    }

    const_iterator end() const
    {
        return _bits->end();
    }

    size_t size() const
    {
        return _bits->size();
    }

    const bool operator[](size_t pos) const
    {
        return _bits->at(pos);
    }

    const std::vector<size_t> indices() const
    {
        return *_indices;
    }

private:
    std::unique_ptr<std::vector<bool>> _bits;
    std::unique_ptr<std::vector<size_t>> _indices;
};

#endif
