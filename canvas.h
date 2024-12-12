#pragma once
using namespace std;

// Canvas
//
// Represents the larger drawing surface that is made up from one or more LEDFeatures

#include "json.hpp"
#include "interfaces.h"
#include "basegraphics.h"
#include "ledfeature.h"
#include "effectsmanager.h"
#include <vector>
#include <mutex>

class Canvas : public ICanvas
{
    static atomic<uint32_t> _nextId;
    uint32_t                _id;
    BaseGraphics            _graphics;
    EffectsManager          _effects;
    string                  _name;
    vector<unique_ptr<ILEDFeature>> _features;
    mutable std::mutex     _featuresMutex;

public:
    Canvas(string name, uint32_t width, uint32_t height, uint16_t fps = 30) : 
        _id(_nextId++),
        _graphics(width, height), 
        _effects(fps),
        _name(name)
    {
    }

    string Name() const override
    {
        return _name;
    }

    uint32_t Id() const override 
    { 
        return _id; 
    }

    uint32_t SetId(uint32_t id) override 
    { 
        _id = id;
        return _id;
    }

    ILEDGraphics & Graphics() override
    {
        return _graphics;
    }
    
    const ILEDGraphics& Graphics() const override 
    { 
        return _graphics; 
    }

    IEffectsManager & Effects() override
    {
        return _effects;
    }

    const IEffectsManager & Effects() const override
    {
        return _effects;
    }

    vector<reference_wrapper<ILEDFeature>> Features() override
    {
        std::lock_guard<std::mutex> lock(_featuresMutex);
        vector<reference_wrapper<ILEDFeature>> features;
        features.reserve(_features.size());
        for_each(_features.begin(), _features.end(),
            [&features](const auto& feature) {
                features.push_back(*feature);
            });
        return features;
    }

    const vector<reference_wrapper<ILEDFeature>> Features() const override
    {
        std::lock_guard<std::mutex> lock(_featuresMutex);
        vector<reference_wrapper<ILEDFeature>> features;
        features.reserve(_features.size());
        for (auto &feature : _features)
            features.push_back(*feature);
        return features;
    }

    uint32_t AddFeature(unique_ptr<ILEDFeature> feature) override
    {
        std::lock_guard<std::mutex> lock(_featuresMutex);
        if (!feature)
            throw invalid_argument("Cannot add a null feature.");

        feature->SetCanvas(this);
        uint32_t id = feature->Id();
        _features.push_back(std::move(feature));
        return id;    
    }

    bool RemoveFeatureById(uint16_t featureId) override
    {
        std::lock_guard<std::mutex> lock(_featuresMutex);
        for (size_t i = 0; i < _features.size(); ++i)
        {
            if (_features[i]->Id() == featureId)
            {
                _features[i]->Socket().Stop();
                _features.erase(_features.begin() + i);
                return true;
            }
        }
        return false;
    }

    friend void to_json(nlohmann::json& j, const ICanvas & canvas);
    friend void from_json(const nlohmann::json& j, unique_ptr<ICanvas>& canvas);
};

// ICanvas --> JSON

inline void to_json(nlohmann::json& j, const ICanvas & canvas) 
{
    j = 
    {
        {"name",              canvas.Name()},
        {"id",                canvas.Id()},
        {"width",             canvas.Graphics().Width()},
        {"height",            canvas.Graphics().Height()},
        {"fps",               canvas.Effects().GetFPS()},
        {"currentEffectName", canvas.Effects().CurrentEffectName()},
        {"features",          canvas.Features()},
        {"effectsManager",    canvas.Effects()}
    };
}

// ICanvas <-- JSON

inline void from_json(const nlohmann::json& j, std::unique_ptr<ICanvas>& canvas) 
{
    // Create canvas with required fields
    canvas = std::make_unique<Canvas>(
        j.at("name").get<std::string>(),
        j.at("width").get<uint32_t>(),
        j.at("height").get<uint32_t>(),
        j.value("fps", 30u) // Default FPS to 30 if not provided
    );

    // Features()
    for (const auto& featureJson : j.value("features", nlohmann::json::array()))
        canvas->AddFeature(std::move(featureJson.get<unique_ptr<ILEDFeature>>()));

    // Validate and deserialize EffectsManager
    if (j.contains("effectsManager")) 
    {
        from_json(j.at("effectsManager"), canvas->Effects());
    }
}
