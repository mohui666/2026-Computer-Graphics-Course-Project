#include "equipment/HydraulicSupport.h"

#include "core/Config.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace mine {

namespace {
std::string supportName(int id) {
    std::ostringstream stream;
    stream << "Hydraulic Support #" << std::setw(2) << std::setfill('0') << id;
    return stream.str();
}

float smoothStep(float value) {
    const float t = std::clamp(value, 0.0F, 1.0F);
    return t * t * (3.0F - 2.0F * t);
}
}

HydraulicSupport::HydraulicSupport(int id, float x, float triggerDelay, float stageDuration)
    : Equipment(100 + id, EquipmentKind::Support, supportName(id)), x_(x),
      triggerDelay_(triggerDelay), stageDuration_(stageDuration) {
    transform_.setPosition({x_, 0.0F, layout::supportCenterZ});
}

void HydraulicSupport::update(float dt) {
    if (lowPressure_) {
        stage_ = SupportStage::LowPressure;
        pressure_ = std::max(8.0F, pressure_ - dt * 12.0F);
        return;
    }
    pressure_ += (32.0F - pressure_) * std::min(1.0F, dt * 2.0F);
    if (stage_ == SupportStage::Normal) return;

    timer_ += std::max(0.0F, dt);
    if (stage_ == SupportStage::Waiting) {
        progress_ = 0.0F;
        if (timer_ >= triggerDelay_) { stage_ = SupportStage::Lowering; timer_ = 0.0F; }
    } else {
        progress_ = std::clamp(timer_ / stageDuration_, 0.0F, 1.0F);
        if (timer_ >= stageDuration_) {
            timer_ = 0.0F;
            progress_ = 0.0F;
            if (stage_ == SupportStage::Lowering) stage_ = SupportStage::Advancing;
            else if (stage_ == SupportStage::Advancing) stage_ = SupportStage::Raising;
            else if (stage_ == SupportStage::Raising) { stage_ = SupportStage::Normal; progress_ = 1.0F; }
        }
    }
}

void HydraulicSupport::triggerAfterPass() {
    if (stage_ == SupportStage::Normal && !lowPressure_) {
        stage_ = SupportStage::Waiting;
        timer_ = 0.0F;
        progress_ = 0.0F;
    }
}

void HydraulicSupport::setLowPressure(bool enabled) {
    lowPressure_ = enabled;
    if (enabled) stage_ = SupportStage::LowPressure;
    else { stage_ = SupportStage::Normal; timer_ = 0.0F; progress_ = 1.0F; }
}

void HydraulicSupport::reset() {
    lowPressure_ = false;
    stage_ = SupportStage::Normal;
    timer_ = 0.0F;
    progress_ = 1.0F;
    pressure_ = 32.0F;
}

SupportVisualPose HydraulicSupport::visualPose() const {
    constexpr float loweredHeight = 0.78F;
    constexpr float lowPressureHeight = 0.82F;
    const float eased = smoothStep(progress_);
    SupportVisualPose pose;

    switch (stage_) {
    case SupportStage::Normal:
    case SupportStage::Waiting:
        break;
    case SupportStage::Lowering:
        pose.heightScale = 1.0F - (1.0F - loweredHeight) * eased;
        break;
    case SupportStage::Advancing:
        pose.heightScale = loweredHeight;
        pose.advanceOffset = -layout::supportAdvanceDistance * eased;
        break;
    case SupportStage::Raising:
        pose.heightScale = loweredHeight + (1.0F - loweredHeight) * eased;
        pose.advanceOffset = -layout::supportAdvanceDistance * (1.0F - eased);
        break;
    case SupportStage::LowPressure:
        break;
    }

    // Pressure falls and recovers continuously, so fault injection and clearing cannot pop the canopy.
    const float pressureRatio = smoothStep((pressure_ - 8.0F) / 24.0F);
    const float pressureHeight = lowPressureHeight + (1.0F - lowPressureHeight) * pressureRatio;
    pose.heightScale = std::min(pose.heightScale, pressureHeight);
    return pose;
}

const char* toString(SupportStage stage) {
    switch (stage) {
    case SupportStage::Normal: return "Normal";
    case SupportStage::Waiting: return "Waiting";
    case SupportStage::Lowering: return "Lowering";
    case SupportStage::Advancing: return "Advancing";
    case SupportStage::Raising: return "Raising";
    case SupportStage::LowPressure: return "Low pressure";
    }
    return "Unknown";
}

} // namespace mine
