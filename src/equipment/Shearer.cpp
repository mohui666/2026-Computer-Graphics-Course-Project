#include "equipment/Shearer.h"
#include "core/Config.h"

#include <algorithm>
#include <cmath>

namespace mine {

namespace {
float approach(float value, float target, float rate, float dt) {
    const float delta = std::clamp(target - value, -rate * dt, rate * dt);
    return value + delta;
}
}

Shearer::Shearer(float minX, float maxX, float speed)
    : Equipment(1, EquipmentKind::Shearer, "SL-800 Double-drum Shearer"), minX_(minX),
      maxX_(maxX), nominalSpeed_(speed), position_(minX) {
    transform_.setPosition({position_, 1.42F, layout::conveyorCenterZ});
    leftArmTransform_.setParent(&transform_);
    rightArmTransform_.setParent(&transform_);
    leftDrumTransform_.setParent(&leftArmTransform_);
    rightDrumTransform_.setParent(&rightArmTransform_);
    updateComponentTransforms();
}

void Shearer::updateComponentTransforms() {
    const float lowOffset = 0.53F, lift = 1.75F;
    // Arm pivots remain attached to the body. Drum offsets create the high/low cutting pair.
    leftArmTransform_.setPosition({-2.65F, 0.45F, -0.15F});
    rightArmTransform_.setPosition({2.65F, 0.45F, -0.15F});
    leftDrumTransform_.setPosition({-2.15F, lowOffset + lift*(1.0F-rightDrumHeight_), -0.82F});
    rightDrumTransform_.setPosition({2.15F, lowOffset + lift*rightDrumHeight_, -0.82F});
}

void Shearer::update(float dt, bool cuttingAllowed) {
    dt = std::max(0.0F, dt);
    const bool active = cuttingAllowed && !overheated_ &&
                        (status_ == ShearerStatus::Cutting || status_ == ShearerStatus::Starting ||
                         status_ == ShearerStatus::Turning);
    const float targetSpeed = active ? nominalSpeed_ : 0.0F;
    speed_ = approach(speed_, targetSpeed, nominalSpeed_ * 2.5F, dt);

    if (active) {
        status_ = ShearerStatus::Cutting;
        position_ += static_cast<float>(direction_) * speed_ * dt;
        drumAngle_ = std::fmod(drumAngle_ + 7.5F * dt, 6.2831853F);
        if (position_ >= maxX_) {
            position_ = maxX_;
            direction_ = -1;
            endpointEvent_ = true;
            status_ = ShearerStatus::Turning;
        } else if (position_ <= minX_) {
            position_ = minX_;
            direction_ = 1;
            endpointEvent_ = true;
            status_ = ShearerStatus::Turning;
        }
    }

    rightDrumHeight_ = approach(rightDrumHeight_, direction_ > 0 ? 1.0F : 0.0F, 1.2F, dt);
    const float phase = position_ * 0.19F;
    const float targetLoad = active ? 68.0F + 12.0F * std::sin(phase) : 0.0F;
    load_ = approach(load_, targetLoad, 45.0F, dt);
    const float targetTemperature = active ? 58.0F + load_ * 0.22F : 34.0F;
    temperature_ = approach(temperature_, targetTemperature, active ? 3.0F : 1.2F, dt);
    if (overheated_) {
        temperature_ = std::max(temperature_, 96.0F);
        speed_ = 0.0F;
        load_ = 0.0F;
        status_ = ShearerStatus::Fault;
    }
    transform_.setPosition({position_, 1.42F, layout::conveyorCenterZ});
    updateComponentTransforms();
}

void Shearer::start() { if (!overheated_) status_ = ShearerStatus::Starting; }
void Shearer::stop() { status_ = ShearerStatus::Stopped; speed_ = 0.0F; }
void Shearer::pause() { if (status_ != ShearerStatus::Fault) status_ = ShearerStatus::Paused; }
void Shearer::resume() { if (!overheated_) status_ = ShearerStatus::Starting; }

void Shearer::reset() {
    position_ = minX_;
    speed_ = 0.0F;
    direction_ = 1;
    drumAngle_ = 0.0F;
    rightDrumHeight_ = 1.0F;
    temperature_ = 34.0F;
    load_ = 0.0F;
    overheated_ = false;
    endpointEvent_ = false;
    status_ = ShearerStatus::Stopped;
    transform_.setPosition({position_, 1.42F, layout::conveyorCenterZ});
    updateComponentTransforms();
}

void Shearer::setOverheated(bool enabled) {
    overheated_ = enabled;
    if (enabled) status_ = ShearerStatus::Fault;
}

bool Shearer::consumedEndpointEvent() {
    const bool value = endpointEvent_;
    endpointEvent_ = false;
    return value;
}

const char* toString(ShearerStatus status) {
    switch (status) {
    case ShearerStatus::Stopped: return "Stopped";
    case ShearerStatus::Starting: return "Starting";
    case ShearerStatus::Cutting: return "Cutting";
    case ShearerStatus::Paused: return "Paused";
    case ShearerStatus::Turning: return "Turning";
    case ShearerStatus::Fault: return "Fault";
    }
    return "Unknown";
}

} // namespace mine
