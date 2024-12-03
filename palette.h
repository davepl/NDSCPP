#pragma once

#include <vector>
#include <cmath>
#include <cstdint>
#include <array>
#include <algorithm>
#include "pixeltypes.h"
#include "palette.h"

template <size_t N>
class Palette
{
protected:
    std::array<CRGB, N> _colorEntries;
    static constexpr bool _IsPowerOfTwo = (N & (N - 1)) == 0;

public:
    bool blend;

    // Constructor taking std::array - existing constructor
    explicit Palette(const std::array<CRGB, N> &colors, bool bBlend = true) noexcept
        : _colorEntries(colors),
          blend(bBlend)
    {
    }

    // Fast compile-time size
    static constexpr size_t originalSize() noexcept { return N; }

    CRGB getColor(double d) const noexcept
    {
        // Normalize d to [0, 1)
        d -= std::floor(d);
        if (d < 0)
            d += 1.0;

        if (!blend)
        {
            if constexpr (_IsPowerOfTwo)
            {
                // Use bitwise AND for power-of-two sizes
                const size_t index = static_cast<size_t>(d * N) & (N - 1);
                return _colorEntries[index];
            }
            else
            {
                const size_t index = static_cast<size_t>(d * N) % N;
                return _colorEntries[index];
            }
        }

        // Calculate position in palette
        const double indexD = d * N;
        const size_t index = static_cast<size_t>(indexD);
        const double fraction = indexD - index;

        // Get colors with wrapped index
        const CRGB &color1 = _colorEntries[index];
        if constexpr (_IsPowerOfTwo)
        {
            const CRGB &color2 = _colorEntries[(index + 1) & (N - 1)];
            return color1.blendWith(color2, fraction);
        }
        else
        {
            const CRGB &color2 = _colorEntries[(index + 1) % N];
            return color1.blendWith(color2, fraction);
        }
    }

    // Fast path for single-precision float, pre-normalized [0,1) input
    CRGB getColorFast(float d) const noexcept
    {
        if (!blend)
        {
            if constexpr (_IsPowerOfTwo)
            {
                return _colorEntries[static_cast<size_t>(d * N) & (N - 1)];
            }
            else
            {
                return _colorEntries[static_cast<size_t>(d * N) % N];
            }
        }

        const float indexF = d * N;
        const size_t index = static_cast<size_t>(indexF);
        const float fraction = indexF - index;

        if constexpr (_IsPowerOfTwo)
        {
            return _colorEntries[index].blendWith(
                _colorEntries[(index + 1) & (N - 1)],
                fraction);
        }
        else
        {
            return _colorEntries[index].blendWith(
                _colorEntries[(index + 1) % N],
                fraction);
        }
    }

    // Define standard palettes as static members
    static const Palette<N> Rainbow;
    static const Palette<N> RainbowStripes;
    static const Palette<N> ChristmasLights;
};

// GaussianPalette
//
// This palette uses a Gaussian distribution to blend colors smoothly.

template <size_t N>
class GaussianPalette : public Palette<N>
{
protected:
    double smoothing = 0.0;
    static constexpr std::array<double, 5> factors = {0.06136, 0.24477, 0.38774, 0.24477, 0.06136};

public:
    // Constructor taking array
    explicit GaussianPalette(const std::array<CRGB, N> &colors) noexcept
        : Palette<N>(colors), smoothing(1.0 / N)
    {
    }

    // Constructor taking initializer list for convenience
    explicit GaussianPalette(std::initializer_list<CRGB> colors) noexcept
        : Palette<N>(colors), smoothing(1.0 / N)
    {
    }

    CRGB getColor(double d) const noexcept override
    {
        const double s = smoothing / N;

        // Pre-calculate all color samples to avoid repeated calls
        std::array<CRGB, 5> samples = {
            Palette<N>::getColor(d - 2 * s),
            Palette<N>::getColor(d - s),
            Palette<N>::getColor(d),
            Palette<N>::getColor(d + s),
            Palette<N>::getColor(d + 2 * s)};

        // Calculate weighted sums for each channel
        double red = 0, green = 0, blue = 0;
        for (size_t i = 0; i < 5; ++i)
        {
            red += samples[i].r * factors[i];
            green += samples[i].g * factors[i];
            blue += samples[i].b * factors[i];
        }

        // Clamp and convert to uint8_t
        return CRGB(
            static_cast<uint8_t>(std::clamp(red, 0.0, 255.0)),
            static_cast<uint8_t>(std::clamp(green, 0.0, 255.0)),
            static_cast<uint8_t>(std::clamp(blue, 0.0, 255.0)));
    }

    // Add the fast path version as well
    CRGB getColorFast(float d) const noexcept
    {
        const float s = static_cast<float>(smoothing / N);

        // Pre-calculate all color samples
        std::array<CRGB, 5> samples = {
            Palette<N>::getColorFast(d - 2 * s),
            Palette<N>::getColorFast(d - s),
            Palette<N>::getColorFast(d),
            Palette<N>::getColorFast(d + s),
            Palette<N>::getColorFast(d + 2 * s)};

        // Calculate weighted sums for each channel
        float red = 0, green = 0, blue = 0;
        for (size_t i = 0; i < 5; ++i)
        {
            red += samples[i].r * static_cast<float>(factors[i]);
            green += samples[i].g * static_cast<float>(factors[i]);
            blue += samples[i].b * static_cast<float>(factors[i]);
        }

        return CRGB(
            static_cast<uint8_t>(std::clamp(red, 0.0f, 255.0f)),
            static_cast<uint8_t>(std::clamp(green, 0.0f, 255.0f)),
            static_cast<uint8_t>(std::clamp(blue, 0.0f, 255.0f)));
    }
};
