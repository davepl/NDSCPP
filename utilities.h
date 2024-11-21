#pragma once

// Utilities
//
// This class provides a number of utility functions that are used by the various
// classes in the project.  Most of them relate to managing the data that is sent
// to the ESP32.  The ESP32 expects a specific format for the data that is sent to
// it, and this class provides functions to convert the data into that format.

#include <vector>
#include <cstdint>
#include <initializer_list>
#include <zlib.h>
#include "pixeltypes.h"

class Utilities
{
public:
    static std::vector<uint8_t> ConvertPixelsToByteArray(const std::vector<CRGB> &pixels, bool reversed, bool redGreenSwap)
    {
        std::vector<uint8_t> byteArray;
        byteArray.reserve(pixels.size() * sizeof(CRGB)); // Each CRGB has 3 components: R, G, B

        // This code makes all kinds of assumptions that CRGB is three RGB bytes, so let's assert that fact
        static_assert(sizeof(CRGB) == 3);

        if (reversed)
        {
            for (auto it = pixels.rbegin(); it != pixels.rend(); ++it)
            {
                if (redGreenSwap)
                {
                    byteArray.push_back(it->g);
                    byteArray.push_back(it->r);
                    byteArray.push_back(it->b);
                }
                else
                {
                    byteArray.push_back(it->r);
                    byteArray.push_back(it->g);
                    byteArray.push_back(it->b);
                }
            }
        }
        else
        {
            for (const auto &pixel : pixels)
            {
                if (redGreenSwap)
                {
                    byteArray.push_back(pixel.g);
                    byteArray.push_back(pixel.r);
                    byteArray.push_back(pixel.b);
                }
                else
                {
                    byteArray.push_back(pixel.r);
                    byteArray.push_back(pixel.g);
                    byteArray.push_back(pixel.b);
                }
            }
        }

        return byteArray;
    }

    // The following XXXXToBytes functions produce a bytestream in the little-endian
    // that the original ESP32 code expects

    // Converts a uint16_t to a vector of bytes in little-endian order
    static constexpr std::vector<uint8_t> WORDToBytes(uint16_t value)
    {
        if constexpr (std::endian::native == std::endian::little)
        {
            return {
                static_cast<uint8_t>(value & 0xFF),
                static_cast<uint8_t>((value >> 8) & 0xFF)};
        }
        else
        {
            return {
                static_cast<uint8_t>(__builtin_bswap16(value) & 0xFF),
                static_cast<uint8_t>((__builtin_bswap16(value) >> 8) & 0xFF)};
        }
    }

    // Converts a uint32_t to a vector of bytes in little-endian order
    static constexpr std::vector<uint8_t> DWORDToBytes(uint32_t value)
    {
        if constexpr (std::endian::native == std::endian::little)
        {
            return {
                static_cast<uint8_t>(value & 0xFF),
                static_cast<uint8_t>((value >> 8) & 0xFF),
                static_cast<uint8_t>((value >> 16) & 0xFF),
                static_cast<uint8_t>((value >> 24) & 0xFF)};
        }
        else
        {
            uint32_t swapped = __builtin_bswap32(value);
            return {
                static_cast<uint8_t>(swapped & 0xFF),
                static_cast<uint8_t>((swapped >> 8) & 0xFF),
                static_cast<uint8_t>((swapped >> 16) & 0xFF),
                static_cast<uint8_t>((swapped >> 24) & 0xFF)};
        }
    }

    // Converts a uint64_t to a vector of bytes in little-endian order
    static constexpr std::vector<uint8_t> ULONGToBytes(uint64_t value)
    {
        if constexpr (std::endian::native == std::endian::little)
        {
            return {
                static_cast<uint8_t>(value & 0xFF),
                static_cast<uint8_t>((value >> 8) & 0xFF),
                static_cast<uint8_t>((value >> 16) & 0xFF),
                static_cast<uint8_t>((value >> 24) & 0xFF),
                static_cast<uint8_t>((value >> 32) & 0xFF),
                static_cast<uint8_t>((value >> 40) & 0xFF),
                static_cast<uint8_t>((value >> 48) & 0xFF),
                static_cast<uint8_t>((value >> 56) & 0xFF)};
        }
        else
        {
            uint64_t swapped = __builtin_bswap64(value);
            return {
                static_cast<uint8_t>(swapped & 0xFF),
                static_cast<uint8_t>((swapped >> 8) & 0xFF),
                static_cast<uint8_t>((swapped >> 16) & 0xFF),
                static_cast<uint8_t>((swapped >> 24) & 0xFF),
                static_cast<uint8_t>((swapped >> 32) & 0xFF),
                static_cast<uint8_t>((swapped >> 40) & 0xFF),
                static_cast<uint8_t>((swapped >> 48) & 0xFF),
                static_cast<uint8_t>((swapped >> 56) & 0xFF)};
        }
    }

    // Combines multiple byte arrays into one.  My masterpiece for the day :-)

    template <typename... Arrays>
    static std::vector<uint8_t> CombineByteArrays(const Arrays &...arrays)
    {
        std::vector<uint8_t> combined;

        // Calculate the total size of the combined array.  Remember you
        // saw it here first!  It's a fold expression.  New to me too.

        size_t totalSize = (arrays.size() + ... + 0);
        combined.reserve(totalSize);

        // Append each array to the combined vector with another fold expression
        (combined.insert(combined.end(), arrays.begin(), arrays.end()), ...);

        return combined;
    }

    // Gets color bytes at a specific offset, handling reversing and RGB swapping
    static std::vector<uint8_t> GetColorBytesAtOffset(const std::vector<CRGB> &LEDs, uint32_t offset, uint32_t count, bool reversed, bool redGreenSwap)
    {
        std::vector<uint8_t> colorBytes;
        if (offset >= LEDs.size())
        {
            return colorBytes;
        }

        uint32_t end = std::min(offset + count, static_cast<uint32_t>(LEDs.size()));
        if (reversed)
        {
            for (int32_t i = end - 1; i >= static_cast<int32_t>(offset); --i)
            {
                AppendColorBytes(colorBytes, LEDs[i], redGreenSwap);
            }
        }
        else
        {
            for (uint32_t i = offset; i < end; ++i)
            {
                AppendColorBytes(colorBytes, LEDs[i], redGreenSwap);
            }
        }
        return colorBytes;
    }

    static std::vector<uint8_t> Compress(const std::vector<uint8_t> &data)
    {
        // Allocate initial buffer size
        constexpr size_t bufferIncrement = 1024;
        std::vector<uint8_t> compressedData(bufferIncrement);

        // Initialize zlib stream
        z_stream stream{};
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;
        stream.opaque = Z_NULL;

        // Set input data
        stream.next_in = const_cast<Bytef *>(data.data());
        stream.avail_in = static_cast<uInt>(data.size());

        // Initialize deflate process with optimal compression level
        if (deflateInit(&stream, Z_BEST_COMPRESSION) != Z_OK)
        {
            throw std::runtime_error("Failed to initialize zlib compression");
        }

        // Compress the data
        int result;
        do
        { // Ensure the output buffer is large enough
            if (stream.total_out >= compressedData.size())
                compressedData.resize(compressedData.size() + bufferIncrement);

            // Set the output buffer
            stream.next_out = compressedData.data() + stream.total_out;
            stream.avail_out = static_cast<uInt>(compressedData.size() - stream.total_out);

            // Perform the compression
            result = deflate(&stream, Z_FINISH);
            if (result == Z_STREAM_ERROR)
            {
                deflateEnd(&stream);
                throw std::runtime_error("Error during zlib compression");
            }
        } while (result != Z_STREAM_END);

        // Finalize and clean up
        deflateEnd(&stream);

        // Resize the vector to the actual size of the compressed data
        compressedData.resize(stream.total_out);

        return compressedData;
    }

private:
    // Helper to append color bytes to a vector
    static void AppendColorBytes(std::vector<uint8_t> &bytes, const CRGB &color, bool redGreenSwap)
    {
        if (redGreenSwap)
        {
            bytes.push_back(color.g);
            bytes.push_back(color.r);
        }
        else
        {
            bytes.push_back(color.r);
            bytes.push_back(color.g);
        }
        bytes.push_back(color.b);
    }
};

