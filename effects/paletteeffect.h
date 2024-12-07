#pragma once

// PaletteEffect
// 
// A versatile effect that scrolls a palette of colors across the canvas.  The effect can be configured
// with a variety of parameters to control the speed, density, and appearance of the scrolling colors.

using namespace std;
using namespace std::chrono;

#include "../serialization.h"
#include "../ledeffectbase.h"
#include "../pixeltypes.h"
#include "../palette.h"

class PaletteEffect : public LEDEffectBase 
{
private:
    double _iPixel = 0;
    double _iColor;

public:
    Palette  _Palette;
    double   _LEDColorPerSecond = 3.0;
    double   _LEDScrollSpeed = 0.0;
    double   _Density = 1.0;
    double   _EveryNthDot = 1.0;
    uint32_t _DotSize = 1;
    bool     _RampedColor = false;
    double   _Brightness = 1.0;
    bool     _Mirrored = false;

    // New constructor taking std::vector directly
    PaletteEffect(const    string & name, 
                  const    vector<CRGB> & colors,
                  double   ledColorPerSecond = 3.0,
                  double   ledScrollSpeed = 0.0,
                  double   density = 1.0,
                  double   everyNthDot = 1.0,
                  uint32_t dotSize = 1,
                  bool     rampedColor = false,
                  double   brightness = 1.0,
                  bool     mirrored = false,
                  bool     bBlend   = true) 
        : LEDEffectBase(name),
          _Palette(colors, bBlend), 
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

    void ToJson(nlohmann::json& j) const override
    {
        to_json(j["palette"], _Palette);  // Use existing palette serializer
        
        j = {
            {"type", "PaletteEffect"},
            {"name", Name()},
            {"palette", j["palette"]},
            {"ledColorPerSecond", _LEDColorPerSecond},
            {"ledScrollSpeed", _LEDScrollSpeed},
            {"density", _Density},
            {"everyNthDot", _EveryNthDot},
            {"dotSize", _DotSize},
            {"rampedColor", _RampedColor},
            {"brightness", _Brightness},
            {"mirrored", _Mirrored}
        };
    }

    static std::unique_ptr<PaletteEffect> FromJson(const nlohmann::json& j)
    {
        // Extract colors from the nested "palette" object
        std::vector<CRGB> colors = j.at("palette").at("colors").get<std::vector<CRGB>>();
        bool blend = j.at("palette").at("blend").get<bool>();

        return std::make_unique<PaletteEffect>(
            j.at("name").get<std::string>(),
            colors,
            j.at("ledColorPerSecond").get<double>(),
            j.at("ledScrollSpeed").get<double>(),
            j.at("density").get<double>(),
            j.at("everyNthDot").get<double>(),
            j.at("dotSize").get<uint32_t>(),
            j.at("rampedColor").get<bool>(),
            j.at("brightness").get<double>(),
            j.at("mirrored").get<bool>(),
            blend  // Pass the blend flag
        );
    }


};