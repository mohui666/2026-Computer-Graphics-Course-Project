#pragma once

#include "equipment/Equipment.h"

namespace mine {

enum class ShearerStatus { Stopped, Starting, Cutting, Paused, Turning, Fault };

class Shearer final : public Equipment {
public:
    Shearer(float minX, float maxX, float speed);

    void update(float dt, bool cuttingAllowed);
    void start();
    void stop();
    void pause();
    void resume();
    void reset();
    void setOverheated(bool enabled);

    [[nodiscard]] float position() const { return position_; }
    [[nodiscard]] float speed() const { return speed_; }
    [[nodiscard]] int direction() const { return direction_; }
    [[nodiscard]] float drumAngle() const { return drumAngle_; }
    [[nodiscard]] float temperature() const { return temperature_; }
    [[nodiscard]] float load() const { return load_; }
    [[nodiscard]] bool overheated() const { return overheated_; }
    [[nodiscard]] bool isCutting() const { return status_ == ShearerStatus::Cutting; }
    [[nodiscard]] bool consumedEndpointEvent();
    [[nodiscard]] ShearerStatus status() const { return status_; }
    [[nodiscard]] const Transform& leftArmTransform() const { return leftArmTransform_; }
    [[nodiscard]] const Transform& rightArmTransform() const { return rightArmTransform_; }
    [[nodiscard]] const Transform& leftDrumTransform() const { return leftDrumTransform_; }
    [[nodiscard]] const Transform& rightDrumTransform() const { return rightDrumTransform_; }

private:
    void updateComponentTransforms();

    float minX_;
    float maxX_;
    float nominalSpeed_;
    float position_;
    float speed_ = 0.0F;
    int direction_ = 1;
    float drumAngle_ = 0.0F;
    float rightDrumHeight_ = 1.0F;
    float temperature_ = 34.0F;
    float load_ = 0.0F;
    bool overheated_ = false;
    bool endpointEvent_ = false;
    ShearerStatus status_ = ShearerStatus::Stopped;
    Transform leftArmTransform_;
    Transform rightArmTransform_;
    Transform leftDrumTransform_;
    Transform rightDrumTransform_;
};

const char* toString(ShearerStatus status);

} // namespace mine
