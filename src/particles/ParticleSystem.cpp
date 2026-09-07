#include "particles/ParticleSystem.h"
#include <algorithm>
#include <cmath>

namespace mine {
void ParticleSystem::update(float dt, bool emit, const glm::vec3& leftDrum, const glm::vec3& rightDrum) {
    if (dt <= 0.0F) return;
    if (emit) {
        accumulator_ = std::min(accumulator_ + dt*100.0F, 20.0F);
        while (accumulator_ >= 1.0F && particles_.size() < 420) {
            accumulator_ -= 1.0F;
            const float a = static_cast<float>((sequence_*47U)%113U)/113.0F;
            const float b = static_cast<float>((sequence_*79U)%127U)/127.0F;
            DustParticle particle;
            particle.mist = sequence_%3U != 0;
            const glm::vec3 drum = sequence_%2U ? leftDrum : rightDrum;
            ++sequence_;
            const float angle = a*6.2831853F;
            particle.position = drum + glm::vec3(std::cos(angle)*1.12F,std::sin(angle)*1.12F,0.46F);
            particle.velocity = {(a-0.5F)*1.5F,particle.mist ? -0.30F-b : 0.25F+b*0.40F,
                                 particle.mist ? -0.65F-b*0.6F : 0.18F+b*0.5F};
            particle.lifetime = particle.mist ? 0.55F+a*0.65F : 2.0F+a*2.0F;
            particle.size = particle.mist ? 0.14F+b*0.16F : 0.22F+b*0.38F;
            particles_.push_back(particle);
        }
    } else accumulator_ = 0.0F;
    for (auto& particle : particles_) {
        particle.age += dt;
        particle.position += particle.velocity*dt;
        if (particle.mist) particle.velocity.y -= dt*1.2F;
        else particle.velocity *= std::max(0.0F,1.0F-dt*0.45F);
        const float life = std::clamp(particle.age/particle.lifetime,0.0F,1.0F);
        particle.alpha = std::min(life*8.0F,1.0F)*(1.0F-life)*(particle.mist?0.28F:0.22F);
    }
    particles_.erase(std::remove_if(particles_.begin(),particles_.end(),
        [](const DustParticle& p){ return p.age>=p.lifetime; }),particles_.end());
}
void ParticleSystem::clear() { particles_.clear(); accumulator_=0.0F; sequence_=0; }
} // namespace mine
