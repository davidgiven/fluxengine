#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/data/fluxmapreader.h"
#include "lib/decoders/fluxdecoder.h"
#include "lib/decoders/decoders.pb.h"

/* This is a port of the samdisk code:
 *
 * https://github.com/simonowen/samdisk/blob/master/src/FluxDecoder.cpp
 *
 * I'm not actually terribly sure how it works, but it does, and much better
 * than my code.
 */

FluxDecoder::FluxDecoder(
    FluxmapReader* fmr, nanoseconds_t bitcell, const DecoderProto& config):
    _fmr(fmr),
    _pll_phase(config.pll_phase()),
    _pll_adjust(config.pll_adjust()),
    _flux_scale(config.flux_scale()),
    _clock(bitcell),
    _clock_centre(bitcell),
    _clock_min(bitcell * (1.0 - _pll_adjust)),
    _clock_max(bitcell * (1.0 + _pll_adjust)),
    _flux(0),
    _leading_zeroes(fmr->tell().zeroes)
{
}

bool FluxDecoder::readBit()
{
    if (_leading_zeroes > 0)
    {
        _leading_zeroes--;
        return false;
    }
    else if (_leading_zeroes == 0)
    {
        _leading_zeroes--;
        return true;
    }

    while (!_fmr->eof() && (_flux < (_clock / 2)))
    {
        _flux += nextFlux() * _flux_scale;
        ;
        _clocked_zeroes = 0;
    }

    _flux -= _clock;
    if (_flux >= (_clock / 2))
    {
        _clocked_zeroes++;
        _goodbits++;
        return false;
    }

    /* PLL adjustment: change the clock frequency according to the phase
     * mismatch */
    if (_clocked_zeroes <= 3)
    {
        /* In sync: adjust base clock */

        _clock += _flux * _pll_adjust;
    }
    else
    {
        /* Out of sync: adjust the base clock back towards the centre */

        _clock += (_clock_centre - _clock) * _pll_adjust;

        /* We require 256 good bits before reporting another sync loss event. */

        if (_goodbits >= 256)
            _sync_lost = true;
        _goodbits = 0;
    }

    /* Clamp the clock's adjustment range. */

    _clock = std::min(std::max(_clock_min, _clock), _clock_max);

    /* I'm not sure what this does, but the original comment is:
     * Authentic PLL: Do not snap the timing window to each flux transition
     */

    _flux = _flux * (1.0 - _pll_phase);

    _goodbits++;
    return true;
}

std::vector<bool> FluxDecoder::readBits(unsigned count)
{
    std::vector<bool> result;
    while (!_fmr->eof() && count--)
    {
        bool b = readBit();
        result.push_back(b);
    }
    return result;
}

std::vector<bool> FluxDecoder::readBits(const Fluxmap::Position& until)
{
    std::vector<bool> result;
    while (!_fmr->eof() && (_fmr->tell().bytes < until.bytes))
    {
        bool b = readBit();
        result.push_back(b);
    }
    return result;
}

nanoseconds_t FluxDecoder::nextFlux()
{
    return _fmr->readInterval(_clock_centre) * NS_PER_TICK;
}
