#pragma once
using namespace std;

// Controller
//
// Represents the entire system, contains the canvases which contain the features which contain the effects.

#include "json.hpp"
#include "interfaces.h"
#include "basegraphics.h"
#include "effectsmanager.h"
#include <vector>
#include "global.h"
#include "canvas.h"
#include "ledfeature.h"
#include "effects/colorwaveeffect.h"
#include "effects/starfield.h"
#include "effects/videoeffect.h"
#include "effects/misceffects.h"
#include "palette.h"
#include "effects/paletteeffect.h"
#include "effects/fireworkseffect.h"

class Controller : public IController
{
  private:

    vector<unique_ptr<ICanvas>> _canvases;
    uint16_t                    _port;

  public:

    Controller(uint16_t port = 7777) : _port(port)
    {
    }

    vector<ICanvas*> Canvases() const override
    {
        vector<ICanvas*> canvases;
        canvases.reserve(_canvases.size());
        for (const auto& canvas : _canvases)
            canvases.push_back(canvas.get());
        return canvases;
    }

    uint16_t GetPort() const override
    {
        return _port;
    }

    bool AddFeatureToCanvas(uint16_t canvasId, unique_ptr<ILEDFeature> feature) override
    {
        logger->debug("Adding feature to canvas {}...", canvasId);

        auto canvas = GetCanvasById(canvasId);
        if (canvas == nullptr)
        {
            logger->error("Canvas {} not found in AddFeatureToCanvas.", canvasId);
            return false;
        }

        canvas->AddFeature(std::move(feature));
        return true;
    }

    bool RemoveFeatureFromCanvas(uint16_t canvasId, uint16_t featureId) override
    {
        logger->debug("Removing feature {} from canvas {}...", featureId, canvasId);
        
        // Find the matching canvas and remove the feature from it
        auto canvas = GetCanvasById(canvasId);
        if (canvas == nullptr)
        {
            logger->error("Canvas {} not found in RemoveFeatureFromCanvas.", canvasId);
            return false;
        }
        return canvas->RemoveFeatureById(featureId);
    }


    // LoadSampleCanvases
    //
    // Until we have full load and save ability, this function will be used to load sample canvases.

    void LoadSampleCanvases() override
    {
        logger->debug("Loading sample canvases...");

        vector<unique_ptr<ICanvas>> canvases;

        // Define a Canvas for the Mesmerizer

        auto canvasMesmerizer = make_unique<Canvas>("Mesmerizer", 64, 32, 20);
        auto feature1 = make_unique<LEDFeature>(
            canvasMesmerizer.get(), // Canvas pointer
            "192.168.8.161",        // Hostname
            "Mesmerizer",           // Friendly Name
            49152,                  // Port
            64, 32,                 // Width, Height
            0, 0,                   // Offset X, Offset Y
            false,                  // Reversed
            0,                      // Channel
            false,                  // Red-Green Swap
            180                     // Client Buffer Count
        );
        canvasMesmerizer->AddFeature(std::move(feature1));
        canvasMesmerizer->Effects().AddEffect(make_unique<MP4PlaybackEffect>("Starfield", "./media/mp4/rickroll.mp4"));
        canvasMesmerizer->Effects().SetCurrentEffect(0, *canvasMesmerizer);
        canvases.push_back(std::move(canvasMesmerizer));

        //---------------------------------------------------------------------

        // Define a Canvas for the Workbench Banner

        auto canvasBanner = make_unique<Canvas>("Banner", 512, 32, 24);
        auto featureBanner = make_unique<LEDFeature>(
            canvasBanner.get(), // Canvas pointer
            "192.168.1.98",     // Hostname
            "Banner",           // Friendly Name
            49152,              // Port∏
            512, 32,            // Width, Height
            0, 0,               // Offset X, Offset Y
            false,              // Reversed
            0,                  // Channel
            false,              // Red-Green Swap
            500);
        canvasBanner->AddFeature(std::move(featureBanner));
        canvasBanner->Effects().AddEffect(make_unique<StarfieldEffect>("Starfield", 100));
        canvasBanner->Effects().SetCurrentEffect(0, *canvasBanner);
        canvases.push_back(std::move(canvasBanner));

        //---------------------------------------------------------------------

        auto canvasWindow1 = make_unique<Canvas>("Window1", 100, 1, 3);
        auto featureWindow1 = make_unique<LEDFeature>(
            canvasWindow1.get(), // Canvas pointer
            "192.168.8.8",       // Hostname
            "Window1",           // Friendly Name
            49152,               // Port∏
            100, 1,              // Width, Height
            0, 0,                // Offset X, Offset Y
            false,               // Reversed
            0,                   // Channel
            false,               // Red-Green Swap
            21);
        canvasWindow1->AddFeature(std::move(featureWindow1));
        canvasWindow1->Effects().AddEffect(make_unique<SolidColorFill>("Yellow Window", CRGB(255, 112, 0)));
        canvasWindow1->Effects().SetCurrentEffect(0, *canvasWindow1);
        canvases.push_back(std::move(canvasWindow1));

        //---------------------------------------------------------------------

        auto canvasWindow2 = make_unique<Canvas>("Window2", 100, 1, 3);
        auto featureWindow2 = make_unique<LEDFeature>(
            canvasWindow2.get(), // Canvas pointer
            "192.168.8.9",       // Hostname
            "Window2",           // Friendly Name
            49152,               // Port∏
            100, 1,              // Width, Height
            0, 0,                // Offset X, Offset Y
            false,               // Reversed
            0,                   // Channel
            false,               // Red-Green Swap
            21);
        canvasWindow2->AddFeature(std::move(featureWindow2));
        canvasWindow2->Effects().AddEffect(make_unique<SolidColorFill>("Blue Window", CRGB::Blue));
        canvasWindow2->Effects().SetCurrentEffect(0, *canvasWindow2);
        canvases.push_back(std::move(canvasWindow2));

        //---------------------------------------------------------------------

        auto canvasWindow3 = make_unique<Canvas>("Window3", 100, 1, 3);
        auto featureWindow3 = make_unique<LEDFeature>(
            canvasWindow3.get(), // Canvas pointer
            "192.168.8.10",      // Hostname
            "Window3",           // Friendly Name
            49152,               // Port∏
            100, 1,              // Width, Height
            0, 0,                // Offset X, Offset Y
            false,               // Reversed
            0,                   // Channel
            false,               // Red-Green Swap
            21);
        canvasWindow3->AddFeature(std::move(featureWindow3));
        canvasWindow3->Effects().AddEffect(make_unique<SolidColorFill>("Green Window", CRGB::Green));
        canvasWindow3->Effects().SetCurrentEffect(0, *canvasWindow3);
        canvases.push_back(std::move(canvasWindow3));

        //---------------------------------------------------------------------

        // Cabinets - The shop cupboards in my shop
        {
            constexpr auto start1 = 0, length1 = 300 + 200;
            constexpr auto start2 = length1, length2 = 300 + 300;
            constexpr auto start3 = length1 + length2, length3 = 144;
            constexpr auto start4 = length1 + length2 + length3, length4 = 144;
            constexpr auto totalLength = length1 + length2 + length3 + length4;

            auto canvasCabinets = make_unique<Canvas>("Cabinets", totalLength, 1, 20);
            auto featureCabinets1 = make_unique<LEDFeature>(
                canvasCabinets.get(), // Canvas pointer
                "192.168.8.12",       // Hostname
                "Cupboard1",          // Friendly Name
                49152,                // Port∏
                length1, 1,           // Width, Height
                start1, 0,            // Offset X, Offset Y
                false,                // Reversed
                0,                    // Channel
                false,                // Red-Green Swap
                180);
            auto featureCabinets2 = make_unique<LEDFeature>(
                canvasCabinets.get(), // Canvas pointer
                "192.168.8.29",       // Hostname
                "Cupboard2",          // Friendly Name
                49152,                // Port∏
                length2, 1,           // Width, Height
                start2, 0,            // Offset X, Offset Y
                false,                // Reversed
                0,                    // Channel
                false,                // Red-Green Swap
                180);
            auto featureCabinets3 = make_unique<LEDFeature>(
                canvasCabinets.get(), // Canvas pointer
                "192.168.8.30",       // Hostname
                "Cupboard3",          // Friendly Name
                49152,                // Port∏
                length3, 1,           // Width, Height
                start3, 0,            // Offset X, Offset Y
                false,                // Reversed
                0,                    // Channel
                false,                // Red-Green Swap
                180);
            auto featureCabinets4 = make_unique<LEDFeature>(
                canvasCabinets.get(), // Canvas pointer
                "192.168.8.15",       // Hostname
                "Cupboard4",          // Friendly Name
                49152,                // Port∏
                length4, 1,           // Width, Height
                start4, 0,            // Offset X, Offset Y
                false,                // Reversed
                0,                    // Channel
                false,                // Red-Green Swap
                180);

            canvasCabinets->AddFeature(std::move(featureCabinets1));
            canvasCabinets->AddFeature(std::move(featureCabinets2));
            canvasCabinets->AddFeature(std::move(featureCabinets3));
            canvasCabinets->AddFeature(std::move(featureCabinets4));
            canvasCabinets->Effects().AddEffect(make_unique<PaletteEffect>("Rainbow Scroll", StandardPalettes::Rainbow, 2.0, 0.0, 0.01));
            canvasCabinets->Effects().SetCurrentEffect(0, *canvasCabinets);
            canvases.push_back(std::move(canvasCabinets));
        }

        // Cabana - Christmas lights that wrap around my guest house
        {
            constexpr auto start1 = 0, length1 = (5 * 144 - 1) + (3 * 144);
            constexpr auto start2 = length1, length2 = 5 * 144 + 55;
            constexpr auto start3 = length1 + length2, length3 = 6 * 144 + 62;
            constexpr auto start4 = length1 + length2 + length3, length4 = 8 * 144 - 23;
            constexpr auto totalLength = length1 + length2 + length3 + length4;

            auto canvasCabana = make_unique<Canvas>("Cabana", totalLength, 1, 24);
            auto featureCabana1 = make_unique<LEDFeature>(
                canvasCabana.get(), // Canvas pointer
                "192.168.8.33",     // Hostname
                "CBWEST",           // Friendly Name
                49152,              // Port∏
                length1, 1,         // Width, Height
                start1, 0,          // Offset X, Offset Y
                false,              // Reversed
                0,                  // Channel
                false,              // Red-Green Swap
                180);
            auto featureCabana2 = make_unique<LEDFeature>(
                canvasCabana.get(), // Canvas pointer
                "192.168.8.5",      // Hostname
                "CBEAST1",          // Friendly Name
                49152,              // Port∏
                length2, 1,         // Width, Height
                start2, 0,          // Offset X, Offset Y
                true,               // Reversed
                0,                  // Channel
                false,              // Red-Green Swap
                180);
            auto featureCabana3 = make_unique<LEDFeature>(
                canvasCabana.get(), // Canvas pointer
                "192.168.8.37",     // Hostname
                "CBEAST2",          // Friendly Name
                49152,              // Port∏
                length3, 1,         // Width, Height
                start3, 0,          // Offset X, Offset Y
                false,              // Reversed
                0,                  // Channel
                false,              // Red-Green Swap
                180);
            auto featureCabana4 = make_unique<LEDFeature>(
                canvasCabana.get(), // Canvas pointer
                "192.168.8.31",     // Hostname
                "CBEAST3",          // Friendly Name
                49152,              // Port∏
                length4, 1,         // Width, Height
                start4, 0,          // Offset X, Offset Y
                false,              // Reversed
                0,                  // Channel
                false,              // Red-Green Swap
                180);

            canvasCabana->AddFeature(std::move(featureCabana1));
            canvasCabana->AddFeature(std::move(featureCabana2));
            canvasCabana->AddFeature(std::move(featureCabana3));
            canvasCabana->AddFeature(std::move(featureCabana4));
            canvasCabana->Effects().AddEffect(make_unique<PaletteEffect>("Rainbow Scroll", StandardPalettes::ChristmasLights, 0.0, 5.0, 1.0, 30, 4));
            canvasCabana->Effects().SetCurrentEffect(0, *canvasCabana);
            canvases.push_back(std::move(canvasCabana));
        }
        {
            auto canvasCeiling = make_unique<Canvas>("Ceiling", 144 * 5 + 38, 1, 30);
            auto featureCeiling = make_unique<LEDFeature>(
                canvasCeiling.get(), // Canvas pointer
                "192.168.8.60",      // Hostname
                "Ceiling",           // Friendly Name
                49152,               // Port
                144 * 5 + 38, 1,     // Width, Height
                0, 0,                // Offset X, Offset Y
                false,               // Reversed
                0,                   // Channel
                false,               // Red-Green Swap
                500                  // Client Buffer Count
            );
            canvasCeiling->AddFeature(std::move(featureCeiling));
            canvasCeiling->Effects().AddEffect(make_unique<FireworksEffect>("Fireworks"));
            canvasCeiling->Effects().SetCurrentEffect(0, *canvasCeiling);
            canvases.push_back(std::move(canvasCeiling));
        }
        {
            auto canvasTree = make_unique<Canvas>("Tree", 32, 1, 30);
            auto featureTree = make_unique<LEDFeature>(
                canvasTree.get(), // Canvas pointer
                "192.168.8.167",  // Hostname
                "Tree",           // Friendly Name
                49152,            // Port
                32, 1,            // Width, Height
                0, 0,             // Offset X, Offset Y
                false,            // Reversed
                0,                // Channel
                false,            // Red-Green Swap
                180               // Client Buffer Count
            );
            canvasTree->AddFeature(std::move(featureTree));
            canvasTree->Effects().AddEffect(make_unique<PaletteEffect>("Rainbow Scroll", StandardPalettes::Rainbow, 0.25, 0.0, 1, 1));
            canvasTree->Effects().SetCurrentEffect(0, *canvasTree);
            canvases.push_back(std::move(canvasTree));
        }

        _canvases = std::move(canvases);
    }

    void Connect() override
    {
        logger->debug("Connecting canvases...");

        for (const auto &canvas : _canvases)
            for (const auto &feature : canvas->Features())
                feature->Socket().Start();
    }

    void Disconnect() override
    {
        logger->debug("Disconnecting canvases...");

        for (const auto &canvas : _canvases)
            for (const auto &feature : canvas->Features())
                feature->Socket().Stop();
    }

    void Start() override
    {
        logger->debug("Starting canvases...");

        for (auto &canvas : _canvases)
            canvas->Effects().Start(*canvas);
    }

    void Stop() override
    {
        logger->debug("Stopping canvases...");

        for (auto &canvas : _canvases)
            canvas->Effects().Stop();
    }

    bool AddCanvas(unique_ptr<ICanvas> ptrCanvas) override
    {
        logger->debug("Adding canvas {}...", ptrCanvas->Name());

        // Check to see if a canvas already uses the ptrCanvas's ID and fail if so
        if (nullptr != GetCanvasById(ptrCanvas->Id()))
        {
            logger->error("Canvas with ID {} already exists.", ptrCanvas->Id());
            return false;
        }

        _canvases.push_back(std::move(ptrCanvas));
        return true;
    }

    bool DeleteCanvasById(uint32_t id) override
    {
        logger->debug("Deleting canvas {}...", id);

        for (size_t i = 0; i < _canvases.size(); ++i)
        {
            if (_canvases[i]->Id() == id)
            {
                _canvases.erase(_canvases.begin() + i);
                return true;
            }
        }

        logger->error("Canvas with ID {} not found in DeleteCanvasById.", id);
        return false;
    }

    bool UpdateCanvas(unique_ptr<ICanvas> ptrCanvas) override
    {
        logger->debug("Updating canvas {}...", ptrCanvas->Name());

        // Find the existing canvas with the matching Id and replace it
        for (size_t i = 0; i < _canvases.size(); ++i)
        {
            if (_canvases[i]->Id() == ptrCanvas->Id())
            {
                _canvases[i] = std::move(ptrCanvas);
                return true;
            }
        }

        logger->error("Canvas with ID {} not found in UpdateCanvas.", ptrCanvas->Id());
        return false;
    }

    ICanvas * GetCanvasById(uint16_t id) const override
    {
        // Find and return the canvas with the matching ID
        for (auto &canvas : _canvases)
            if (canvas->Id() == id)
                return canvas.get();

        logger->error("Canvas with ID {} not found in GetCanvasById.", id);
        return nullptr;
    }

    vector<const ISocketChannel *> GetSockets() const override
    {
        vector<const ISocketChannel *> sockets;
        for (const auto &canvas : _canvases)
            for (const auto &feature : canvas->Features())
                sockets.push_back(&feature->Socket());
        return sockets;
    }

    const ISocketChannel * GetSocketById(uint16_t id) const override
    {
        for (auto &canvas : _canvases)
            for (auto &feature : canvas->Features())
                if (feature->Socket().Id() == id)
                    return &feature->Socket();

        logger->error("Socket with ID {} not found in GetSocketById.", id);                     
        return nullptr;
    }
};

