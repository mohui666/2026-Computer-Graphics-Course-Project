#pragma once

#include <glm/glm.hpp>

namespace mine {

enum class CameraPreset { Overview = 1, Shearer = 2, Supports = 3, Conveyor = 4, Entrance = 5, Top = 6 };

class Camera {
public:
    Camera();

    [[nodiscard]] glm::mat4 viewMatrix() const;
    [[nodiscard]] glm::mat4 projectionMatrix(float aspect) const;
    [[nodiscard]] glm::vec3 forward() const;
    [[nodiscard]] glm::vec3 right() const;
    [[nodiscard]] glm::vec3 screenRay(float mouseX, float mouseY, int width, int height) const;

    void move(const glm::vec3& localDirection, float dt, bool fast);
    void rotate(float deltaX, float deltaY);
    void applyPreset(CameraPreset preset, float shearerX = 0.0F);
    void focus(const glm::vec3& target);
    void reset();

    [[nodiscard]] const glm::vec3& position() const { return position_; }
    [[nodiscard]] float fov() const { return fov_; }

private:
    glm::vec3 position_{0.0F, 7.0F, 22.0F};
    float yaw_ = -90.0F;
    float pitch_ = -14.0F;
    float fov_ = 52.0F;
};

} // namespace mine
