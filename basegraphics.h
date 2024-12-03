#pragma once
using namespace std;

// BaseGraphics
//
// A class that can do all the basic drawing functions (pixel, line, circle, etc) of the ILEDGraphics 
// interface to its own internal buffer of pixels.  It manipulates the buffer only through SetPixel

#include <vector>
#include "pixeltypes.h" // Assuming this defines the CRGB structure
#include "interfaces.h"

class BaseGraphics : public ILEDGraphics
{
protected:
    uint32_t _width;
    uint32_t _height;
    vector<CRGB> _pixels;

    virtual inline __attribute__((always_inline)) uint32_t _index(uint32_t x, uint32_t y) const 
    {
        return y * _width + x;
    }

public:
    explicit BaseGraphics(uint32_t width, uint32_t height) 
        : _width(width), _height(height), _pixels(width * height) 
    {

    }

    // ICanvas methods
    uint32_t Width()  const override { return _width; }
    uint32_t Height() const override { return _height; }

    const vector<CRGB>& GetPixels() const override
    {
        return _pixels;
    }

    void SetPixel(uint32_t x, uint32_t y, const CRGB& color) override
    {
        if (x < _width && y < _height)
            _pixels[_index(x, y)] = color;
    }

    CRGB GetPixel(uint32_t x, uint32_t y) const override 
    {
        if (x < _width && y < _height)
            return _pixels[_index(x, y)];
        return CRGB(0, 0, 0); // Default to black for out-of-bounds
    }

    void Clear(const CRGB& color = CRGB::Black) override
    {
        FillRectangle(0, 0, _width, _height, color);
    }

    void FillRectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, const CRGB& color) override
    {
        for (int j = y; j < y + height; ++j)
            for (int i = x; i < x + width; ++i)
                SetPixel(i, j, color);
    }

    void DrawLine(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, const CRGB& color) override
    {
        int32_t dx = abs((int32_t)x2 - (int32_t)x1), dy = abs((int32_t)y2 - (int32_t)y1);
        int32_t sx = (x1 < x2) ? 1 : -1, sy = (y1 < y2) ? 1 : -1;
        int32_t err = dx - dy;

        while (true) 
        {
            SetPixel(x1, y1, color);
            if (x1 == x2 && y1 == y2) break;
            int e2 = 2 * err;
            if (e2 > -dy) 
            {
                err -= dy;
                x1 += sx;
            }
            if (e2 < dx) 
            {
                err += dx;
                y1 += sy;
            }
        }
    }

    void DrawCircle(uint32_t x, uint32_t y, uint32_t radius, const CRGB& color) override
    {
        uint32_t cx = 0, cy = radius;
        int32_t  d = 1 - radius;

        while (cy >= cx) 
        {
            SetPixel(x + cx, y + cy, color);
            SetPixel(x - cx, y + cy, color);
            SetPixel(x + cx, y - cy, color);
            SetPixel(x - cx, y - cy, color);
            SetPixel(x + cy, y + cx, color);
            SetPixel(x - cy, y + cx, color);
            SetPixel(x + cy, y - cx, color);
            SetPixel(x - cy, y - cx, color);

            ++cx;
            if (d < 0)
            {
                d += 2 * cx + 1;
            }
            else 
            {
                --cy;
                d += 2 * (cx - cy) + 1;
            }
        }
    }

    void FillCircle(uint32_t x, uint32_t y, uint32_t radius, const CRGB& color) override
    {
        // Fill within the bounding box, but only those pixels within radius of the center

        for (int32_t cy = -radius; cy <= radius; ++cy)
            for (int32_t cx = -radius; cx <= radius; ++cx)
                if (cx * cx + cy * cy <= radius * radius)
                    SetPixel(x + cx, y + cy, color);
    }

    void DrawRectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, const CRGB& color) override
    {
        DrawLine(x, y, x + width - 1, y, color);                            // Top
        DrawLine(x, y, x, y + height - 1, color);                           // Left
        DrawLine(x + width - 1, y, x + width - 1, y + height - 1, color);   // Right
        DrawLine(x, y + height - 1, x + width - 1, y + height - 1, color);  // Bottom
    }

    void FadeFrameBy(uint8_t dimAmount) override
    {
        for (auto& pixel : _pixels)
        {
            pixel.r = scale8(pixel.r, 255-dimAmount);
            pixel.g = scale8(pixel.g, 255-dimAmount);
            pixel.b = scale8(pixel.b, 255-dimAmount);
        }
    }

    void SetPixelsF(float fPos, float count, CRGB c, bool bMerge = false) override
    {
        // Early exit for empty ranges or out-of-bounds start positions
        if (count <= 0 || fPos >= _pixels.size() || fPos + count <= 0)
            return;

        // Pre-calculate common values
        const size_t arraySize = _pixels.size();
        const int startIdx = std::max(0, static_cast<int>(std::floor(fPos)));
        const int endIdx = std::min(static_cast<int>(arraySize), static_cast<int>(std::ceil(fPos + count)));
        const float frac1 = fPos - std::floor(fPos);
        const uint8_t fade1 = static_cast<uint8_t>((std::max(frac1, 1.0f - count)) * 255);
        float remainingCount = count - (1.0f - frac1);
        const int fullPixels = static_cast<int>(remainingCount);
        const float lastFrac = remainingCount - std::floor(remainingCount);
        const uint8_t fade2 = static_cast<uint8_t>((1.0f - lastFrac) * 255);

        if (!bMerge)
        {
            // Non-merging implementation
            
            if (startIdx >= 0 && startIdx < arraySize)
            {
                CRGB c1 = c;
                c1.fadeToBlackBy(fade1);
                _pixels[startIdx] = c1;
            }

            // Middle pixels - use pointer arithmetic for speed
            CRGB *pixel = &_pixels[startIdx + 1];
            const CRGB *end = &_pixels[endIdx - 1];
            while (pixel < end)
                *pixel++ = c;

            // Last pixel if needed
            if (lastFrac > 0 && endIdx - 1 < arraySize)
            {
                CRGB c2 = c;
                c2.fadeToBlackBy(fade2);
                _pixels[endIdx - 1] = c2;
            }
        }
        else
        {
            // Merging implementation
            // First pixel
            if (startIdx >= 0 && startIdx < arraySize)
            {
                CRGB c1 = c;
                c1.fadeToBlackBy(fade1);
                _pixels[startIdx] += c1;
            }

            // Middle pixels - use pointer arithmetic for speed
            CRGB *pixel = &_pixels[startIdx + 1];
            const CRGB *end = &_pixels[endIdx - 1];
            while (pixel < end)
                *pixel++ += c;

            // Last pixel if needed
            if (lastFrac > 0 && endIdx - 1 < arraySize)
            {
                CRGB c2 = c;
                c2.fadeToBlackBy(fade2);
                _pixels[endIdx - 1] += c2;
            }
        }
    }
};
