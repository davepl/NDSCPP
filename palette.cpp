#include "palette.h"
#include "pixeltypes.h"

// Palette.cpp
//
// Contains the implementation of the Palette class defined in palette.h.

// Rainbow - 8 colors for smoother transitions in a performant power of 2 size

template<>
const Palette<8> Palette<8>::Rainbow({
    CRGB(255, 0, 0),     // Pure Red       (~650nm)
    CRGB(255, 80, 0),    // Red-Orange     (~620nm)
    CRGB(255, 165, 0),   // Orange         (~590nm)
    CRGB(255, 255, 0),   // Yellow         (~570nm)
    CRGB(0, 255, 0),     // Pure Green     (~530nm)
    CRGB(0, 150, 255),   // Blue-Green     (~500nm)
    CRGB(0, 0, 255),     // Pure Blue      (~470nm)
    CRGB(143, 0, 255)    // Violet         (~420nm)
});

// Christmas Lights - 4 colors (power of 2 size!)

template<>
const Palette<4> Palette<4>::ChristmasLights({
    CRGB::Red,    // Red
    CRGB::Green,  // Green
    CRGB::Blue,   // Blue
    CRGB::Purple  // Purple
});

// Rainbow Stripes - 16 colors (power of 2 size!)

template<>
const Palette<16> Palette<16>::RainbowStripes({
    CRGB::Black, 
    CRGB::Red,
    CRGB::Black, 
    CRGB::Orange,   
    CRGB::Black, 
    CRGB::Yellow,   
    CRGB::Black, 
    CRGB::Green,
    CRGB::Black, 
    CRGB::Cyan,
    CRGB::Black, 
    CRGB::Blue,
    CRGB::Black, 
    CRGB::Purple,   
    CRGB::Black, 
    CRGB::Green
});