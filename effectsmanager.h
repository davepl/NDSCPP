#pragma once
using namespace std;
using namespace std::chrono;

// EffectsManager
//
// Manages a collection of ILEDEffect objects.  The EffectsManager is responsible for
// starting and stopping the effects, and for switching between them.  The EffectsManager
// can also be used to clear all effects.

#include "interfaces.h"
#include <vector>
#include <memory>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <atomic>

class EffectsManager : public IEffectsManager
{
public:
    EffectsManager()
        : _currentEffectIndex(-1), _running(false) // No effect selected initially
    {}

    ~EffectsManager()
    {
        Stop(); // Ensure the worker thread is stopped when the manager is destroyed
    }

    // Add an effect to the manager
    void AddEffect(shared_ptr<ILEDEffect> effect)
    {
        if (!effect)
            throw invalid_argument("Cannot add a null effect.");
        _effects.push_back(effect);

        // Automatically set the first effect as current if none is selected
        if (_currentEffectIndex == -1)
            _currentEffectIndex = 0;
    }

    // Remove an effect from the manager
    void RemoveEffect(shared_ptr<ILEDEffect> effect)
    {
        if (!effect)
            throw invalid_argument("Cannot remove a null effect.");

        auto it = remove(_effects.begin(), _effects.end(), effect);
        if (it != _effects.end())
        {
            auto index = distance(_effects.begin(), it);
            _effects.erase(it);

            // Adjust the current effect index
            if (index <= _currentEffectIndex)
                _currentEffectIndex = (_currentEffectIndex > 0) ? _currentEffectIndex - 1 : -1;

            // If no effects remain, reset the current index
            if (_effects.empty())
                _currentEffectIndex = -1;
        }
    }

    // Start the current effect
    void StartCurrentEffect(ICanvas& canvas)
    {
        if (IsEffectSelected())
            _effects[_currentEffectIndex]->Start(canvas);
    }

    void SetCurrentEffect(size_t index, ICanvas& canvas)
    {
        if (index >= _effects.size())
            throw out_of_range("Effect index out of range.");

        _currentEffectIndex = index;

        StartCurrentEffect(canvas);
   }

    // Update the current effect and render it to the canvas
    void UpdateCurrentEffect(ICanvas& canvas, milliseconds millisDelta)
    {
        if (IsEffectSelected())
            _effects[_currentEffectIndex]->Update(canvas, millisDelta);
    }

    // Switch to the next effect
    void NextEffect()
    {
        if (!_effects.empty())
            _currentEffectIndex = (_currentEffectIndex + 1) % _effects.size();
    }

    // Switch to the previous effect
    void PreviousEffect()
    {
        if (!_effects.empty())
            _currentEffectIndex = (_currentEffectIndex == 0) ? _effects.size() - 1 : _currentEffectIndex - 1;
    }

    // Get the name of the current effect
    string CurrentEffectName() const
    {
        if (IsEffectSelected())
            return _effects[_currentEffectIndex]->Name();
        return "No Effect Selected";
    }

    // Clear all effects
    void ClearEffects()
    {
        _effects.clear();
        _currentEffectIndex = -1;
    }

    // Start the worker thread to update effects
    void Start(ICanvas& canvas, ISocketController& socketController)
    {
        if (_running.exchange(true))
            return; // Already running

        _workerThread = thread([this, &canvas, &socketController]() 
        {
            auto lastTime = steady_clock::now();
            while (_running)
            {
                auto now = steady_clock::now();
                auto deltaTime = duration_cast<milliseconds>(now - lastTime);
                lastTime = now;

                UpdateCurrentEffect(canvas, deltaTime);
                
                // Now Enqueue the frames from each feature to the SocketController

                for (const auto &feature : canvas.Features())
                {
                    auto frame = feature->GetDataFrame();
                    auto target = socketController.FindChannelByHost(feature->HostName());
                    if (!target)
                        throw runtime_error("Feature host not found in SocketController.");
                    auto compressedFrame = target->CompressFrame(frame);
                    target->EnqueueFrame(compressedFrame);
                }

                this_thread::sleep_for(milliseconds(33)); // ~30 FPS
            }
        });
    }

    // Stop the worker thread
    void Stop()
    {
        if (!_running.exchange(false))
            return; // Not running

        if (_workerThread.joinable())
            _workerThread.join();
    }

private:
    vector<shared_ptr<ILEDEffect>> _effects;
    int _currentEffectIndex; // Index of the current effect
    thread _workerThread;
    atomic<bool> _running;

    bool IsEffectSelected() const
    {
        return _currentEffectIndex >= 0 && _currentEffectIndex < static_cast<int>(_effects.size());
    }
};
