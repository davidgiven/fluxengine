#ifndef FLUXDECODER_H
#define FLUXDECODER_H

class FluxmapReader;

class FluxDecoder
{
public:
    FluxDecoder(
        FluxmapReader* fmr, nanoseconds_t bitcell, const DecoderProto& config);

    bool readBit();
    std::vector<bool> readBits(unsigned count);
    std::vector<bool> readBits(const Fluxmap::Position& until);

    std::vector<bool> readBits()
    {
        return readBits(UINT_MAX);
    }

private:
    nanoseconds_t nextFlux();

private:
    FluxmapReader* _fmr;
    double _pll_phase;
    double _pll_adjust;
    double _flux_scale;
    nanoseconds_t _clock = 0;
    nanoseconds_t _clock_centre;
    nanoseconds_t _clock_min;
    nanoseconds_t _clock_max;
    nanoseconds_t _flux = 0;
    unsigned _clocked_zeroes = 0;
    unsigned _goodbits = 0;
    bool _index = false;
    bool _sync_lost = false;
    int _leading_zeroes;
};

#endif
