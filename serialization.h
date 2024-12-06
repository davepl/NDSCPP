#pragma once

using namespace std;
using namespace std::chrono;

// Note that in theory, for proper separation of concerns, these functions should NOT need
// access to headers that are not part of the interfaces - i.e., everything we need should
// be on an interface

#include "json.hpp"
#include "interfaces.h"
#include "ledfeature.h"
#include "canvas.h"
#include "palette.h"

//
// CRGB serialization
// 

inline void to_json(nlohmann::json& j, const CRGB& color) 
{
    j = {
        {"r", color.r},
        {"g", color.g},
        {"b", color.b}
    };
}

inline void from_json(const nlohmann::json& j, CRGB& color) 
{
    color = CRGB(
        j.at("r").get<uint8_t>(),
        j.at("g").get<uint8_t>(),
        j.at("b").get<uint8_t>()
    );
}

//
// Palette serialization
//

inline void to_json(nlohmann::json& j, const Palette & palette) 
{
    auto colorsJson = nlohmann::json::array();
    for (const auto& color : palette.getColors()) 
        colorsJson.push_back(color);  // Uses CRGB serializer
        
    j = 
    {
        {"colors", colorsJson},
        {"blend", palette._bBlend}
    };
}

inline void from_json(const nlohmann::json& j, Palette & palette) 
{
    // Deserialize the "colors" array
    std::vector<CRGB> colors;
    for (const auto& colorJson : j.at("colors")) 
        colors.push_back(colorJson.get<CRGB>()); // Use CRGB's from_json function

    // Deserialize the "blend" flag, defaulting to true if not present
    bool blend = j.value("blend", true);

    // Assign to palette (Palette must allow reassignment)
    palette = Palette(colors, blend);
}

//
// ClientResponse serialization
//

inline void to_json(nlohmann::json &j, const ClientResponse &response)
{
    j ={
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

inline void from_json(const nlohmann::json& j, ClientResponse& response) 
{
    response.size = j.at("responseSize").get<uint8_t>();
    response.sequence = j.at("sequenceNumber").get<uint32_t>();
    response.flashVersion = j.at("flashVersion").get<uint32_t>();
    response.currentClock = j.at("currentClock").get<uint64_t>();
    response.oldestPacket = j.at("oldestPacket").get<uint64_t>();
    response.newestPacket = j.at("newestPacket").get<uint64_t>();
    response.brightness = j.at("brightness").get<uint8_t>();
    response.wifiSignal = j.at("wifiSignal").get<int8_t>();
    response.bufferSize = j.at("bufferSize").get<uint32_t>();
    response.bufferPos = j.at("bufferPos").get<uint32_t>();
    response.fpsDrawing = j.at("fpsDrawing").get<float>();
    response.watts = j.at("watts").get<float>();
}

//
// Feature serialization
//

inline void to_json(nlohmann::json& j, const ILEDFeature & feature) 
{
    j = {
            {"type", "LEDFeature"},
            {"id", feature.Id()},
            {"hostName", feature.Socket().HostName()},
            {"friendlyName", feature.Socket().FriendlyName()},
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
            {"reconnectCount", feature.Socket().GetReconnectCount()}
        };

    const auto &response = feature.Socket().LastClientResponse();
    if (response.size == sizeof(ClientResponse))
        j["lastClientResponse"] = response;
}

inline void from_json(const nlohmann::json& j, std::unique_ptr<ILEDFeature>& feature) 
{
    if (j.at("type").get<std::string>() != "LEDFeature") 
    {
        throw std::runtime_error("Invalid feature type in JSON");
    }

    feature = std::make_unique<LEDFeature>(
        nullptr,  // Canvas pointer needs to be set after deserialization
        j.at("hostName").get<std::string>(),
        j.at("friendlyName").get<std::string>(),
        j.at("port").get<uint16_t>(),
        j.at("width").get<uint32_t>(),
        j.value("height", 1u),
        j.value("offsetX", 0u),
        j.value("offsetY", 0u),
        j.value("reversed", false),
        j.value("channel", uint8_t(0)),
        j.value("redGreenSwap", false),
        j.value("clientBufferCount", 8u)
    );
}


//
// Canvas serialization
//

inline void to_json(nlohmann::json& j, const ICanvas & canvas) 
{
    j = {
        {"name", canvas.Name()},
        {"id", canvas.Id()},
        {"width", canvas.Graphics().Width()},
        {"height", canvas.Graphics().Height()},
        {"fps", canvas.Effects().GetFPS()},
        {"currentEffectName", canvas.Effects().CurrentEffectName()}
    };

    // Add features array
    auto featuresJson = nlohmann::json::array();
    for (const auto& feature : canvas.Features()) {
        nlohmann::json featureJson;
        to_json(featureJson, *feature);
        featuresJson.push_back(featureJson);
    }
    j["features"] = featuresJson;
}

inline void from_json(const nlohmann::json& j, std::unique_ptr<ICanvas>& canvas) 
{
    // Create canvas with required fields
    canvas = std::make_unique<Canvas>(
        j.at("name").get<std::string>(),
        j.at("width").get<uint32_t>(),
        j.at("height").get<uint32_t>(),
        j.value("fps", 30u)
    );

    // Deserialize features if present
    if (j.contains("features")) {
        for (const auto& featureJson : j["features"])
            canvas->AddFeature(featureJson.get<std::unique_ptr<ILEDFeature>>());
    }
}

//
// ISocketChannel serialization
//

inline void to_json(nlohmann::json &j, const ISocketChannel & socket)
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
        j["id"] = socket.Id();
        
        // Note: featureId and canvasId can't be included here since they're not
        // properties of the socket itself but rather of its container objects

        const auto &lastResponse = socket.LastClientResponse();
        if (lastResponse.size == sizeof(ClientResponse))
            j["stats"] = lastResponse; // Uses the ClientResponse serializer
    }
    catch (const std::exception &e)
    {
        j = nullptr;
    }
}

inline void from_json(const nlohmann::json& j, unique_ptr<ISocketChannel>& socket) 
{
    socket = make_unique<SocketChannel>(
        j.at("hostName").get<string>(),
        j.at("friendlyName").get<string>(),
        j.value("port", uint16_t(49152))
    );
}

inline void to_json(nlohmann::json &j, const IController &controller)
{
    try
    {
        j["port"] = controller.GetPort();
        j["canvases"] = nlohmann::json::array();
        
        for (const auto * ptrCanvas : controller.Canvases())
        {
            nlohmann::json canvasJson;
            to_json(canvasJson, *ptrCanvas);           // Uses the Canvas serializer
            j["canvases"].push_back(canvasJson);
        }
    }
    catch (const std::exception &e)
    {
        j = nullptr;
    }
}

