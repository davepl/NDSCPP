#pragma once
using namespace std;

// LEDEffectBase
// 
// A helper class that implements the ILEDEffect interface.  

#include "interfaces.h"

class LEDEffectBase : public ILEDEffect
{
protected:
    string _name;

public:
    LEDEffectBase(const string& name) : _name(name) {}

    virtual ~LEDEffectBase() = default;

    const string& Name() const override { return _name; }

    // Default implementation for Start does nothing
    void Start(ICanvas& canvas) override 
    {
    }

    // Default implementation for Update does nothing
    void Update(ICanvas& canvas, milliseconds deltaTime) override 
    {
    }

    virtual void ToJson(nlohmann::json& j) const override
    {
        j["name"] = _name;
    }

    /*
    static std::unique_ptr<LEDEffectBase> FromJson(const nlohmann::json& j)
    {
        return nullptr;
    }
    */
};
