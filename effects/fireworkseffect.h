#pragma once

using namespace std;
using namespace std::chrono;

#include "../interfaces.h"
#include "../ledeffectbase.h"
#include "../pixeltypes.h"
#include "../utilities.h"
#include <vector>
#include <random>
#include <cmath>
#include <queue>

class FireworksEffect : public LEDEffectBase
{
private:
    struct Particle
    {
        CRGB _starColor;
        double _birthTime;
        double _lastUpdate;
        double _velocity;
        double _position;

        Particle(const CRGB& starColor, double pos, double maxSpeed, std::mt19937& rng)
            : _starColor(starColor),
              _position(pos),
              _velocity(Utilities::RandomDouble(-maxSpeed, maxSpeed)),
              _birthTime(GetCurrentTime()),
              _lastUpdate(GetCurrentTime())
        {
        }

        double Age() const
        {
            return GetCurrentTime() - _birthTime;
        }

        void Update(double deltaTime)
        {
            _position += _velocity * deltaTime;
            _lastUpdate = GetCurrentTime();
            _velocity -= 2 * _velocity * deltaTime;
            _starColor.fadeToBlackBy(Utilities::RandomDouble(0.0, 0.1));
        }

    private:
        static double GetCurrentTime()
        {
            return static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count()) / 1000.0;
        }
    };

    std::vector<CRGB> _palette;
    std::queue<Particle> _particles;
    std::mt19937 _rng;

    double _maxSpeed = 175.0;
    double _newParticleProbability = 1.0;
    double _particlePreignitionTime = 0.0;
    double _particleIgnition = 0.2;
    double _particleHoldTime = 0.0;
    double _particleFadeTime = 2.0;
    double _particleSize = 1.0;

public:
    FireworksEffect(const std::string& name) : LEDEffectBase(name), _rng(std::random_device{}()) {}

    void Update(ICanvas& canvas, milliseconds deltaTime) override 
    {
        const auto ledCount = canvas.Graphics().Width() * canvas.Graphics().Height();

        for (int i = 0; i < std::max(5, static_cast<int>(ledCount / 50)); ++i)
        {
            if (Utilities::RandomDouble(0.0, 1.0) < _newParticleProbability * 0.005)
            {
                double startPos = Utilities::RandomDouble(0.0, static_cast<double>(canvas.Graphics().Width()));
                CRGB color = CHSV(Utilities::RandomInt(0, 255), 255, 255);
                int particleCount = Utilities::RandomInt(10, 50);
                double multiplier = Utilities::RandomDouble(1.0, 3.0);

                for (int j = 0; j < particleCount; ++j)
                {
                    _particles.emplace(color, startPos, _maxSpeed * multiplier, _rng);
                }
            }
        }

        while (_particles.size() > ledCount)
        {
            _particles.pop();
        }

        //canvas.Graphics().Clear(CRGB::Black);
        canvas.Graphics().FadeFrameBy(64);

        auto particleIter = _particles.front();
        while (!_particles.empty() && particleIter.Age() > _particleHoldTime + _particleIgnition + _particleFadeTime)
        {
            _particles.pop();
            particleIter = _particles.front();
        }

        std::queue<Particle> newParticles;
        while (!_particles.empty())
        {
            Particle particle = _particles.front();
            _particles.pop();

            particle.Update(deltaTime.count() / 1000.0);
            CRGB color = particle._starColor;

            double fade = 0.0;
            if (particle.Age() < _particleIgnition + _particlePreignitionTime)
            {
                color = CRGB::White;
            }
            else
            {
                double age = particle.Age();
                if (age > _particleHoldTime + _particleIgnition)
                {
                    fade = (age - _particleHoldTime - _particleIgnition) / _particleFadeTime;
                }
                color.fadeToBlackBy(fade);
            }

            _particleSize = max(1.0, (1.0 - fade) * (ledCount / 500.0));
            canvas.Graphics().SetPixelsF(particle._position, _particleSize, color);

            newParticles.push(particle);
        }
        _particles.swap(newParticles);
    }
};

