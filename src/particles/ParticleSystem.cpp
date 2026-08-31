#include "particles/ParticleSystem.h"
#include "core/Config.h"

#include <algorithm>
#include <cmath>

namespace mine {

void ParticleSystem::update(float dt, bool emit, float shearerX, int direction) {
    if (emit) {
        accumulator_ += dt * 24.0F;
        while (accumulator_ >= 1.0F && particles_.size() < 160) {
            accumulator_ -= 1.0F;
            const float a = static_cast<float>((sequence_ * 47U) % 113U) / 113.0F;
            const float b = static_cast<float>((sequence_ * 79U) % 127U) / 127.0F;
            ++sequence_;
            DustParticle particle;
            particle.position = {shearerX - static_cast<float>(direction) * (2.3F + a), 1.0F + b * 1.8F,
                                 layout::coalFaceSurfaceZ + 0.25F + a * 0.55F};
            particle.velocity = {(a - 0.5F) * 0.7F, 0.25F + b * 0.55F, (b - 0.5F) * 0.8F};
            particle.lifetime = 2.0F + a * 2.8F;
            particle.size = 0.08F + b * 0.22F;
            particles_.push_back(particle);
        }
    }
    for (auto& particle : particles_) {
        particle.age += dt;
        particle.position += particle.velocity * dt;
        particle.velocity *= std::max(0.0F, 1.0F - dt * 0.35F);
        particle.alpha = std::clamp(1.0F - particle.age / particle.lifetime, 0.0F, 1.0F) * 0.42F;
    }
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(),
                                    [](const DustParticle& p) { return p.age >= p.lifetime; }),
                     particles_.end());
}

void ParticleSystem::clear() { particles_.clear(); accumulator_ = 0.0F; }

} // namespace mine
