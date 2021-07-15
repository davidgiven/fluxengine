#ifndef TRACK_H
#define TRACK_H

class Fluxmap;
class FluxSource;
class AbstractDecoder;

class Track
{
public:
    Track(unsigned track, unsigned side):
        physicalTrack(track),
        physicalSide(side)
    {}

public:
    unsigned physicalTrack;
    unsigned physicalSide;
	FluxSource* fluxsource;
    std::unique_ptr<Fluxmap> fluxmap;

    std::vector<Sector> sectors;
};

typedef std::vector<std::unique_ptr<Track>> TrackVector;

#endif
