#include "graphics/Camera.h"
#include "core/Config.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace mine {

Camera::Camera() { applyPreset(CameraPreset::Overview); }

glm::vec3 Camera::forward() const {
    const float yaw = glm::radians(yaw_);
    const float pitch = glm::radians(pitch_);
    return glm::normalize(glm::vec3(std::cos(yaw) * std::cos(pitch), std::sin(pitch),
                                    std::sin(yaw) * std::cos(pitch)));
}

glm::vec3 Camera::right() const { return glm::normalize(glm::cross(forward(), glm::vec3(0, 1, 0))); }

glm::mat4 Camera::viewMatrix() const { return glm::lookAt(position_, position_ + forward(), {0, 1, 0}); }

glm::mat4 Camera::projectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(fov_), std::max(aspect, 0.01F), 0.08F, 260.0F);
}

glm::vec3 Camera::screenRay(float mouseX, float mouseY, int width, int height) const {
    const float x = 2.0F * mouseX / static_cast<float>(std::max(1, width)) - 1.0F;
    const float y = 1.0F - 2.0F * mouseY / static_cast<float>(std::max(1, height));
    const glm::mat4 inverse = glm::inverse(projectionMatrix(static_cast<float>(width) / std::max(1, height)) * viewMatrix());
    glm::vec4 world = inverse * glm::vec4(x, y, 1.0F, 1.0F);
    world /= world.w;
    return glm::normalize(glm::vec3(world) - position_);
}

void Camera::move(const glm::vec3& localDirection, float dt, bool fast) {
    const float speed = fast ? 18.0F : 7.0F;
    glm::vec3 flatForward = glm::normalize(glm::vec3(forward().x, 0.0F, forward().z));
    position_ += (right() * localDirection.x + glm::vec3(0, 1, 0) * localDirection.y +
                  flatForward * localDirection.z) * speed * dt;
}

void Camera::rotate(float deltaX, float deltaY) {
    yaw_ += deltaX * 0.12F;
    pitch_ = std::clamp(pitch_ + deltaY * 0.12F, -88.0F, 88.0F);
}

void Camera::focus(const glm::vec3& target) {
    const glm::vec3 direction = glm::normalize(target - position_);
    pitch_ = glm::degrees(std::asin(direction.y));
    yaw_ = glm::degrees(std::atan2(direction.z, direction.x));
}

void Camera::applyPreset(CameraPreset preset, float shearerX) {
    fov_ = 52.0F;
    switch (preset) {
    case CameraPreset::Overview:
        position_={-40.5F,2.40F,-1.60F}; focus({-22.0F,2.80F,-3.80F}); break;
    case CameraPreset::Shearer:
        fov_=62.0F;
        position_={shearerX-10.0F,3.05F,-2.62F};
        focus({shearerX,2.75F,-4.55F}); break;
    case CameraPreset::Supports:
        fov_=63.0F;
        position_={-10.0F,1.85F,-3.04F}; focus({2.0F,3.05F,-1.50F}); break;
    case CameraPreset::Conveyor:
        fov_=57.0F;
        position_={shearerX+9.5F,1.78F,-2.84F};
        focus({shearerX,1.35F,layout::conveyorCenterZ}); break;
    case CameraPreset::Entrance:
        fov_=60.0F;
        position_={-42.0F,2.25F,-0.40F}; focus({-23.0F,2.70F,-3.90F}); break;
    case CameraPreset::Top:
        fov_ = 52.0F;
        position_ = {-25.0F, 46.0F, 38.0F};
        focus({0.0F, 0.4F, -1.2F});
        break;
    }
}

void Camera::reset() { applyPreset(CameraPreset::Overview); }

} // namespace mine
