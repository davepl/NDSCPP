#pragma once
using namespace std;

// Canvas
//
// Represents the larger drawing surface that is made up from one or more LEDFeatures

#include "json.hpp"
#include "interfaces.h"
#include "basegraphics.h"
#include "effectsmanager.h"
#include <vector>

class Canvas : public ICanvas
{
    static atomic<uint32_t> _nextId;
    uint32_t                _id;
    BaseGraphics            _graphics;
    EffectsManager          _effects;
    string                  _name;
    vector<unique_ptr<ILEDFeature>> _features;

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
        vector<reference_wrapper<ILEDFeature>> features;
        features.reserve(_features.size());
        std::for_each(_features.begin(), _features.end(),
            [&features](const auto& feature) {
                features.push_back(*feature);
            });
        return features;
    }

    const vector<reference_wrapper<ILEDFeature>> Features() const override
    {
        vector<reference_wrapper<ILEDFeature>> features;
        for (auto &feature : _features)
            features.push_back(*feature);
        return features;
    }

    uint32_t AddFeature(unique_ptr<ILEDFeature> feature) override
    {
        if (!feature)
            throw invalid_argument("Cannot add a null feature.");

        uint32_t id = feature->Id();
        _features.push_back(std::move(feature));
        return id;    
    }

    bool RemoveFeatureById(uint16_t featureId) override
    {
        for (size_t i = 0; i < _features.size(); ++i)
        {
            if (_features[i]->Id() == featureId)
            {
                _features.erase(_features.begin() + i);
                return true;
            }
        }
        return false;
    }    
};
