#pragma once

#include "equipment/Equipment.h"

namespace mine {

enum class SupportStage { Normal, Waiting, Lowering, Advancing, Raising, LowPressure };

struct SupportVisualPose {
    float heightScale = 1.0F;
    float advanceOffset = 0.0F;
};

class HydraulicSupport final : public Equipment {
public:
    HydraulicSupport(int id, float x, float triggerDelay, float stageDuration);

    void update(float dt);
    void triggerAfterPass();
    void setLowPressure(bool enabled);
    void reset();

    [[nodiscard]] SupportStage stage() const { return stage_; }
    [[nodiscard]] float progress() const { return progress_; }
    [[nodiscard]] float pressure() const { return pressure_; }
    [[nodiscard]] float x() const { return x_; }
    [[nodiscard]] bool lowPressure() const { return lowPressure_; }
    [[nodiscard]] SupportVisualPose visualPose() const;

private:
    float x_;
    float triggerDelay_;
    float stageDuration_;
    float timer_ = 0.0F;
    float progress_ = 1.0F;
    float pressure_ = 32.0F;
    bool lowPressure_ = false;
    SupportStage stage_ = SupportStage::Normal;
};

const char* toString(SupportStage stage);

} // namespace mine
