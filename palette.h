#pragma once

#include <vector>
#include <cmath>
#include <cstdint>
#include <array>
#include <algorithm>
#include "pixeltypes.h"

class Palette
{
protected:
    std::vector<CRGB> _colorEntries;

public:
    bool _bBlend = true;

    static const std::vector<CRGB> Rainbow;
    static const std::vector<CRGB> RainbowStripes;
    static const std::vector<CRGB> ChristmasLights;

    explicit Palette(const std::vector<CRGB> & colors, bool bBlend = true) 
        : _colorEntries(colors), _bBlend(bBlend)
    {
    }

    // Add copy/move operations
    Palette(const Palette& other) 
        : _colorEntries(other._colorEntries)
        , _bBlend(other._bBlend)
    {
    }

    Palette& operator=(const Palette& other) 
    {
        _colorEntries = other._colorEntries;
        _bBlend = other._bBlend;
        return *this;
    }

    // Optional but good to have
    Palette(Palette&& other) noexcept = default;
    Palette& operator=(Palette&& other) noexcept = default;

    size_t originalSize() const
    {
        return _colorEntries.size();
    }

    const std::vector<CRGB> & getColors() const
    {
        return _colorEntries;
    }

    virtual CRGB getColor(double d) const 
    {
        auto N = _colorEntries.size();

        // Normalize d to [0, 1)
        d -= std::floor(d);
        if (d < 0) d += 1.0;

        if (!_bBlend)
        {
            return _colorEntries[static_cast<size_t>(d * N) % N];
        }

        // Calculate position in palette
        const double indexD = d * N;
        const size_t index = static_cast<size_t>(indexD);
        const double fraction = indexD - index;

        // Get colors with wrapped index
        const CRGB& color1 = _colorEntries[index];
        const CRGB& color2 = _colorEntries[(index + 1) % N];
        return color1.blendWith(color2, fraction);
    }

    // Fast path for single-precision float, pre-normalized [0,1) input
    virtual CRGB getColorFast(float d) const 
    {
        auto N = _colorEntries.size();
        if (!_bBlend)
        {
            return _colorEntries[static_cast<size_t>(d * N) % N];
        }

        const float indexF = d * N;
        const size_t index = static_cast<size_t>(indexF);
        const float fraction = indexF - index;
        
        return _colorEntries[index].blendWith(_colorEntries[(index + 1) % N], fraction);
    }
};

