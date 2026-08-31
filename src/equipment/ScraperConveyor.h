#pragma once

#include "equipment/Equipment.h"

#include <vector>

namespace mine {

enum class ConveyorStatus { Stopped, Starting, Running, Jammed };

class ScraperConveyor final : public Equipment {
public:
    explicit ScraperConveyor(float nominalSpeed);

    void update(float dt, int coalPieces);
    void start();
    void stop();
    void setSpeedScale(float scale);
    void setJammed(bool enabled);
    void reset();

    [[nodiscard]] ConveyorStatus status() const { return status_; }
    [[nodiscard]] float speed() const { return speed_; }
    [[nodiscard]] float speedScale() const { return speedScale_; }
    [[nodiscard]] float chainPhase() const { return chainPhase_; }
    [[nodiscard]] float load() const { return load_; }
    [[nodiscard]] bool jammed() const { return jammed_; }
    [[nodiscard]] bool isRunning() const { return status_ == ConveyorStatus::Running; }
    [[nodiscard]] std::vector<float> scraperPositions(float faceLength) const;

private:
    float nominalSpeed_;
    float speedScale_ = 1.0F;
    float speed_ = 0.0F;
    float chainPhase_ = 0.0F;
    float load_ = 0.0F;
    bool jammed_ = false;
    ConveyorStatus status_ = ConveyorStatus::Stopped;
};

const char* toString(ConveyorStatus status);

} // namespace mine
