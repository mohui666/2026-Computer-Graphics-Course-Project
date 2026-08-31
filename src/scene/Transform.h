#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace mine {

class Transform {
public:
    void setParent(Transform* parent);
    void setPosition(const glm::vec3& position);
    void setRotation(const glm::quat& rotation);
    void setScale(const glm::vec3& scale);

    [[nodiscard]] const glm::vec3& position() const { return position_; }
    [[nodiscard]] glm::mat4 localMatrix() const;
    [[nodiscard]] glm::mat4 worldMatrix() const;
    [[nodiscard]] glm::vec3 worldPosition() const;

private:
    Transform* parent_ = nullptr;
    glm::vec3 position_{0.0F};
    glm::quat rotation_{1.0F, 0.0F, 0.0F, 0.0F};
    glm::vec3 scale_{1.0F};
};

} // namespace mine
