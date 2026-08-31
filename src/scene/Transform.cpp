#include "scene/Transform.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <stdexcept>

namespace mine {

void Transform::setParent(Transform* parent) {
    if (parent == this) {
        throw std::invalid_argument("A transform cannot parent itself");
    }
    parent_ = parent;
}

void Transform::setPosition(const glm::vec3& position) { position_ = position; }
void Transform::setRotation(const glm::quat& rotation) { rotation_ = glm::normalize(rotation); }
void Transform::setScale(const glm::vec3& scale) { scale_ = scale; }

glm::mat4 Transform::localMatrix() const {
    return glm::translate(glm::mat4(1.0F), position_) * glm::toMat4(rotation_) *
           glm::scale(glm::mat4(1.0F), scale_);
}

glm::mat4 Transform::worldMatrix() const {
    return parent_ ? parent_->worldMatrix() * localMatrix() : localMatrix();
}

glm::vec3 Transform::worldPosition() const { return glm::vec3(worldMatrix()[3]); }

} // namespace mine
