#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace mine {

struct DustParticle {
    glm::vec3 position{0.0F};
    glm::vec3 velocity{0.0F};
    float age = 0.0F;
    float lifetime = 1.0F;
    float size = 0.1F;
    float alpha = 1.0F;
    bool mist = false;
};

class ParticleSystem {
public:
    void update(float dt, bool emit, const glm::vec3& leftDrum, const glm::vec3& rightDrum);
    void clear();
    [[nodiscard]] const std::vector<DustParticle>& particles() const { return particles_; }

private:
    std::vector<DustParticle> particles_;
    float accumulator_ = 0.0F;
    unsigned int sequence_ = 0;
};

} // namespace mine
