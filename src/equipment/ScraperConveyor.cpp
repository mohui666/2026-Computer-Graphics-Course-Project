#include "equipment/ScraperConveyor.h"
#include "core/Config.h"

#include <algorithm>
#include <cmath>

namespace mine {

ScraperConveyor::ScraperConveyor(float nominalSpeed)
    : Equipment(2, EquipmentKind::Conveyor, "SGZ Scraper Conveyor"), nominalSpeed_(nominalSpeed) {
    transform_.setPosition({0.0F, 0.2F, layout::conveyorCenterZ});
}

void ScraperConveyor::update(float dt, int coalPieces) {
    const float target = (status_ == ConveyorStatus::Starting || status_ == ConveyorStatus::Running) &&
                                 !jammed_
                             ? nominalSpeed_ * speedScale_
                             : 0.0F;
    const float maxDelta = nominalSpeed_ * std::max(0.0F, dt) * 2.0F;
    speed_ += std::clamp(target - speed_, -maxDelta, maxDelta);
    if (jammed_) status_ = ConveyorStatus::Jammed;
    else if (status_ == ConveyorStatus::Starting && speed_ >= target * 0.9F) status_ = ConveyorStatus::Running;
    chainPhase_ = std::fmod(chainPhase_ + speed_ * std::max(0.0F, dt), layout::scraperSpacing);
    const float targetLoad = std::min(100.0F, static_cast<float>(coalPieces) * 1.7F);
    load_ += (targetLoad - load_) * std::min(1.0F, dt * 2.0F);
}

void ScraperConveyor::start() { if (!jammed_) status_ = ConveyorStatus::Starting; }
void ScraperConveyor::stop() { status_ = ConveyorStatus::Stopped; speed_ = 0.0F; }
void ScraperConveyor::setSpeedScale(float scale) { speedScale_ = std::clamp(scale, 0.2F, 2.0F); }
void ScraperConveyor::setJammed(bool enabled) {
    jammed_ = enabled;
    if (enabled) { status_ = ConveyorStatus::Jammed; speed_ = 0.0F; }
    else status_ = ConveyorStatus::Stopped;
}

void ScraperConveyor::reset() {
    speedScale_ = 1.0F;
    speed_ = 0.0F;
    chainPhase_ = 0.0F;
    load_ = 0.0F;
    jammed_ = false;
    status_ = ConveyorStatus::Stopped;
}

std::vector<float> ScraperConveyor::scraperPositions(float faceLength) const {
    const float safeLength = std::max(layout::scraperSpacing, faceLength - 1.6F);
    const int count = std::max(1, static_cast<int>(std::floor(safeLength / layout::scraperSpacing)));
    const float cycleLength = static_cast<float>(count) * layout::scraperSpacing;
    const float start = -cycleLength * 0.5F;
    std::vector<float> positions;
    positions.reserve(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index) {
        float offset = static_cast<float>(index) * layout::scraperSpacing - chainPhase_;
        if (offset < 0.0F) offset += cycleLength;
        positions.push_back(start + offset);
    }
    return positions;
}

const char* toString(ConveyorStatus status) {
    switch (status) {
    case ConveyorStatus::Stopped: return "Stopped";
    case ConveyorStatus::Starting: return "Starting";
    case ConveyorStatus::Running: return "Running";
    case ConveyorStatus::Jammed: return "Jammed";
    }
    return "Unknown";
}

} // namespace mine
