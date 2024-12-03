#pragma once

// PaletteEffect
// 
// A versatile effect that scrolls a palette of colors across the canvas.  The effect can be configured
// with a variety of parameters to control the speed, density, and appearance of the scrolling colors.

using namespace std;
using namespace std::chrono;

#include "../interfaces.h"
#include "../ledeffectbase.h"
#include "../pixeltypes.h"
#include "../palette.h"

template<size_t N>
class PaletteEffect : public LEDEffectBase 
{
private:
    double _iPixel = 0;
    double _iColor;

public:
    Palette<N>  _Palette;
    double   _LEDColorPerSecond = 3.0;
    double   _LEDScrollSpeed = 0.0;
    double   _Density = 1.0;
    double   _EveryNthDot = 1.0;
    uint32_t _DotSize = 1;
    bool     _RampedColor = false;
    double   _Brightness = 1.0;
    bool     _Mirrored = false;

    // New constructor taking std::array directly
    PaletteEffect(const string& name, 
                  const    array<CRGB, N>& colors,
                  double   ledColorPerSecond = 3.0,
                  double   ledScrollSpeed = 0.0,
                  double   density = 1.0,
                  double   everyNthDot = 1.0,
                  uint32_t dotSize = 1,
                  bool     rampedColor = false,
                  double   brightness = 1.0,
                  bool     mirrored = false,
                  bool     bBlend   = true) noexcept
        : LEDEffectBase(name),
          _Palette(Palette<N>(colors, bBlend)), 
          _iColor(0),
          _LEDColorPerSecond(ledColorPerSecond),
          _LEDScrollSpeed(ledScrollSpeed),
          _Density(density),
          _EveryNthDot(everyNthDot),
          _DotSize(dotSize),
          _RampedColor(rampedColor),
          _Brightness(brightness),
          _Mirrored(mirrored)
    {
    }
    
    void Update(ICanvas& canvas, milliseconds deltaTime) override 
    {
        auto& graphics = canvas.Graphics();
        const auto width = graphics.Width();
        const auto height = graphics.Height();
        const auto dotcount = width * height;
        
        graphics.Clear(CRGB::Black);

        // Pre-calculate constants
        const double secondsElapsed = deltaTime.count() / 1000.0;
        const double cPixelsToScroll = secondsElapsed * _LEDScrollSpeed;
        const double cColorsToScroll = secondsElapsed * _LEDColorPerSecond;
        const uint32_t cLength = (_Mirrored ? dotcount / 2 : dotcount);
        const double cCenter = dotcount / 2.0;
        const double colorIncrement = _Density / _Palette.originalSize();
        const double fadeFactor = 1.0 - _Brightness;
        
        // Update state variables
        _iPixel = fmod(_iPixel + cPixelsToScroll, dotcount);
        _iColor = fmod(_iColor + (cColorsToScroll * _Density), 1.0);
        
        // Draw the scrolling color "dots"

        double iColor = _iColor;
        for (double i = 0; i < cLength; i += _EveryNthDot) 
        {
            double iPixel = fmod(i + _iPixel, cLength);
            CRGB c = _Palette.getColor(iColor).fadeToBlackBy(fadeFactor);
            
            graphics.SetPixelsF(iPixel + (_Mirrored ? cCenter : 0), _DotSize, c);
            if (_Mirrored) 
                graphics.SetPixelsF(cCenter - iPixel, _DotSize, c);
           
            iColor = fmod(iColor + colorIncrement, 1.0);
        }
        
        // Handle pixel 0 flicker prevention
        if (dotcount > 1) {
            graphics.SetPixel(0, 0, graphics.GetPixel(1, 0));
        }
    }
};