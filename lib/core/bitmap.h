#ifndef BITMAP_H
#define BITMAP_H

class Agg2D;

class Bitmap
{
public:
    Bitmap(const std::string filename, unsigned width, unsigned height);

    Agg2D& painter();
    void save();

private:
    std::vector<uint8_t> _bitmap;
    std::unique_ptr<Agg2D> _painter;

public:
    std::string filename;
    unsigned width;
    unsigned height;
    bool initialised : 1;
};

#endif
