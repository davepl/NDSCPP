#pragma once

using namespace std;
using namespace std::chrono;

// Serialization
//
// To be able to create a JSON representation of the LEDFeature object, we need to provide
// a to_json function that converts the object into a JSON object.  This function is then
// used by the nlohmann::json library to serialize the object.

#include "json.hpp"
#include "interfaces.h"

inline static double ByteSwapDouble(double value)
{
    // Helper function to swap bytes in a double
    uint64_t temp;
    memcpy(&temp, &value, sizeof(double)); // Copy bits of double to temp
    temp = __builtin_bswap64(temp);        // Byte swap the 64-bit integer
    memcpy(&value, &temp, sizeof(double)); // Copy bits back to double
    return value;
}

// Note that in theory, for proper separation of concerns, these functions should NOT need
// access to headers that are not part of the interfaces - i.e., everything we need should
// be on an interface

// ClientResponse
//
// Response data sent back to server every time we receive a packet.
// This struct is packed to match the exact network protocol format used by ESP32 clients.
// The packed attribute is required to ensure correct network communication but may cause
// alignment issues on some architectures.

struct OldClientResponse
{
    uint32_t size;         // 4
    uint32_t flashVersion; // 4
    double currentClock;   // 8
    double oldestPacket;   // 8
    double newestPacket;   // 8
    double brightness;     // 8
    double wifiSignal;     // 8
    uint32_t bufferSize;   // 4
    uint32_t bufferPos;    // 4
    uint32_t fpsDrawing;   // 4
    uint32_t watts;        // 4
} __attribute__((packed)); // Packed attribute required for network protocol compatibility

struct ClientResponse
{
    uint32_t size = sizeof(ClientResponse);         // 4
    uint64_t sequence = 0;                          // 8
    uint32_t flashVersion = 0;                      // 4
    double currentClock = 0;                        // 8
    double oldestPacket = 0;                        // 8
    double newestPacket = 0;                        // 8
    double brightness = 0;                          // 8
    double wifiSignal = 0;                          // 8
    uint32_t bufferSize = 0;                        // 4
    uint32_t bufferPos = 0;                         // 4
    uint32_t fpsDrawing = 0;                        // 4
    uint32_t watts = 0;                             // 4

    ClientResponse& operator=(const OldClientResponse& old)
    {
        size = sizeof(ClientResponse);;
        sequence = 0;  // New field, initialize to 0
        flashVersion = old.flashVersion;
        currentClock = old.currentClock;
        oldestPacket = old.oldestPacket;
        newestPacket = old.newestPacket;
        brightness = old.brightness;
        wifiSignal = old.wifiSignal;
        bufferSize = old.bufferSize;
        bufferPos = old.bufferPos;
        fpsDrawing = old.fpsDrawing;
        watts = old.watts;
        return *this;
    }

    // Member function to translate the structure from the ESP32 little endian
    // to whatever the current running system is

    void TranslateClientResponse()
    {
        // Check the system's endianness
        if constexpr (std::endian::native == std::endian::little)
            return; // No-op for little-endian systems

        // Perform byte swaps for big-endian systems
        size = __builtin_bswap32(size);
        sequence = __builtin_bswap64(sequence); // Added missing sequence swap
        flashVersion = __builtin_bswap32(flashVersion);
        currentClock = ByteSwapDouble(currentClock);
        oldestPacket = ByteSwapDouble(oldestPacket);
        newestPacket = ByteSwapDouble(newestPacket);
        brightness = ByteSwapDouble(brightness);
        wifiSignal = ByteSwapDouble(wifiSignal);
        bufferSize = __builtin_bswap32(bufferSize);
        bufferPos = __builtin_bswap32(bufferPos);
        fpsDrawing = __builtin_bswap32(fpsDrawing);
        watts = __builtin_bswap32(watts);
    }
} __attribute__((packed)); // Packed attribute required for network protocol compatibility

// Helper function to serialize ClientResponse stats consistently
inline void SerializeClientResponseStats(nlohmann::json &j, const ClientResponse &response)
{
    j =
        {
            {"responseSize", response.size},
            {"sequenceNumber", response.sequence},
            {"flashVersion", response.flashVersion},
            {"currentClock", response.currentClock},
            {"oldestPacket", response.oldestPacket},
            {"newestPacket", response.newestPacket},
            {"brightness", response.brightness},
            {"wifiSignal", response.wifiSignal},
            {"bufferSize", response.bufferSize},
            {"bufferPos", response.bufferPos},
            {"fpsDrawing", response.fpsDrawing},
            {"watts", response.watts}
        };
}

inline void to_json(nlohmann::json &j, const ClientResponse &response)
{
    SerializeClientResponseStats(j, response);
}

inline void to_json(nlohmann::json &j, const ILEDFeature &feature)
{
    try
    {
        // Safely access Socket() first
        auto &socket = feature.Socket();

        // Manually serialize fields from the ILEDFeature interface
        j = nlohmann::json
        {
            {"hostName", socket.HostName()},
            {"friendlyName", socket.FriendlyName()},
            {"port", feature.Socket().Port()},
            {"width", feature.Width()},
            {"height", feature.Height()},
            {"offsetX", feature.OffsetX()},
            {"offsetY", feature.OffsetY()},
            {"reversed", feature.Reversed()},
            {"channel", feature.Channel()},
            {"redGreenSwap", feature.RedGreenSwap()},
            {"clientBufferCount", feature.ClientBufferCount()},
            {"timeOffset", feature.TimeOffset()},
            {"bytesPerSecond", feature.Socket().GetLastBytesPerSecond()},
            {"isConnected", feature.Socket().IsConnected()},
            {"queueDepth", feature.Socket().GetCurrentQueueDepth()},
            {"queueMaxSize", feature.Socket().GetQueueMaxSize()},
        };

        const auto &response = socket.LastClientResponse();
        if (response.size == sizeof(ClientResponse))
        {
            j["lastClientResponse"] = response;
        }
    }
    catch (const std::exception &e)
    {
        // Log error or handle gracefully
        j = nullptr;
    }
}

inline void to_json(nlohmann::json &j, const ICanvas &canvas)
{
    try
    {
        // Serialize the features vector as an array of JSON objects
        vector<nlohmann::json> featuresJson;
        for (const auto &feature : canvas.Features())
        {
            nlohmann::json featureJson;
            to_json(featureJson, *feature);
            featuresJson.push_back(std::move(featureJson));
        }

        j = nlohmann::json{
            {"width", canvas.Graphics().Width()},
            {"height", canvas.Graphics().Height()},
            {"name", canvas.Name()},
            {"fps", canvas.Effects().GetFPS()},
            {"features", featuresJson}};
    }
    catch (const std::exception &e)
    {
        j = nullptr;
    }
}

inline void to_json(nlohmann::json &j, const ISocketChannel &socket)
{
    try
    {
        j["hostName"] = socket.HostName();
        j["friendlyName"] = socket.FriendlyName();
        j["isConnected"] = socket.IsConnected();
        j["reconnectCount"] = socket.GetReconnectCount();
        j["queueDepth"] = socket.GetCurrentQueueDepth();
        j["queueMaxSize"] = socket.GetQueueMaxSize();
        j["bytesPerSecond"] = socket.GetLastBytesPerSecond();
        j["port"] = socket.Port();
        
        // Note: featureId and canvasId can't be included here since they're not
        // properties of the socket itself but rather of its container objects

        const auto &lastResponse = socket.LastClientResponse();
        if (lastResponse.size == sizeof(ClientResponse))
        {
            j["stats"] = lastResponse; // Uses the ClientResponse serializer
        }
    }
    catch (const std::exception &e)
    {
        j = nullptr;
    }
}
