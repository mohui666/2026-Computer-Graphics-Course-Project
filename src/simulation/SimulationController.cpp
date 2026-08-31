#include "simulation/SimulationController.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace mine {
namespace {
SimulationConfig sanitizedConfig(SimulationConfig config) {
    config.sanitize();
    return config;
}
}

SimulationController::SimulationController(SimulationConfig config)
    : config_(sanitizedConfig(config)),
      shearer_(-config_.faceLength * 0.45F, config_.faceLength * 0.45F, config_.shearerSpeed),
      conveyor_(config_.conveyorSpeed) {
    const float minX = -config_.faceLength * 0.45F;
    const float maxX = config_.faceLength * 0.45F;
    const float spacing = (maxX - minX) / static_cast<float>(config_.supportCount - 1);
    supports_.reserve(static_cast<std::size_t>(config_.supportCount));
    for (int i = 0; i < config_.supportCount; ++i) {
        supports_.emplace_back(i + 1, minX + spacing * static_cast<float>(i),
                               config_.supportTriggerDelay + 0.08F * static_cast<float>(i % 3),
                               config_.supportStageDuration);
    }
    supportTriggered_.assign(supports_.size(), false);
    eventLog_.add(0.0, LogLevel::Info, "System", "Simulation controller created");
}

bool SimulationController::canTransition(SimulationState to) const {
    if (to == SimulationState::EmergencyStopped) return state_ != SimulationState::EmergencyStopped;
    if (state_ == SimulationState::EmergencyStopped) return to == SimulationState::Stopped && !emergencyLatched_;
    if (to == SimulationState::Fault || to == SimulationState::Warning) return true;
    switch (state_) {
    case SimulationState::Stopped: return to == SimulationState::Initializing;
    case SimulationState::Initializing: return to == SimulationState::Ready || to == SimulationState::Stopped;
    case SimulationState::Ready: return to == SimulationState::StartingConveyor || to == SimulationState::Stopped;
    case SimulationState::StartingConveyor:
        return to == SimulationState::CuttingForward || to == SimulationState::Stopped ||
               to == SimulationState::Paused;
    case SimulationState::CuttingForward:
    case SimulationState::CuttingBackward:
        return to == SimulationState::EndTransition || to == SimulationState::Paused ||
               to == SimulationState::Stopped;
    case SimulationState::EndTransition:
        return to == SimulationState::CuttingForward || to == SimulationState::CuttingBackward ||
               to == SimulationState::Paused || to == SimulationState::Stopped;
    case SimulationState::Paused:
        return to == resumeState_ || to == SimulationState::Stopped;
    case SimulationState::Warning:
        return to == SimulationState::Ready || to == SimulationState::StartingConveyor ||
               to == SimulationState::CuttingForward || to == SimulationState::CuttingBackward ||
               to == SimulationState::Stopped;
    case SimulationState::Fault: return to == SimulationState::Stopped || to == SimulationState::Ready;
    case SimulationState::EmergencyStopped: return false;
    }
    return false;
}

void SimulationController::transitionTo(SimulationState next, std::string reason) {
    if (next == state_) return;
    if (!canTransition(next)) {
        eventLog_.add(simulationTime_, LogLevel::Warning, "StateMachine",
                      std::string("Rejected transition ") + toString(state_) + " -> " + toString(next));
        return;
    }
    previousState_ = state_;
    state_ = next;
    stateEnteredAt_ = simulationTime_;
    transitionReason_ = std::move(reason);
    eventLog_.add(simulationTime_, next == SimulationState::Fault ? LogLevel::Error : LogLevel::Info,
                  "StateMachine", std::string(toString(previousState_)) + " -> " + toString(state_) +
                                      ": " + transitionReason_);
}

void SimulationController::initialize() {
    if (emergencyLatched_) {
        eventLog_.add(simulationTime_, LogLevel::Safety, "Safety", "Initialization blocked: emergency stop latched");
        return;
    }
    if (state_ != SimulationState::Stopped) reset();
    transitionTo(SimulationState::Initializing, "Operator initialization");
}

void SimulationController::start() {
    if (state_ != SimulationState::Ready || hasCriticalFault() || emergencyLatched_) return;
    conveyor_.start();
    transitionTo(SimulationState::StartingConveyor, "Start command; conveyor interlock first");
}

void SimulationController::pause() {
    if (state_ == SimulationState::Paused || state_ == SimulationState::Stopped ||
        state_ == SimulationState::EmergencyStopped) return;
    resumeState_ = state_;
    shearer_.pause();
    transitionTo(SimulationState::Paused, "Operator pause");
}

void SimulationController::resume() {
    if (state_ != SimulationState::Paused || emergencyLatched_ || hasCriticalFault()) return;
    shearer_.resume();
    transitionTo(resumeState_, "Operator resume");
}

void SimulationController::stop() {
    if (state_ == SimulationState::EmergencyStopped && emergencyLatched_) return;
    shearer_.stop();
    conveyor_.stop();
    if (state_ != SimulationState::Stopped) transitionTo(SimulationState::Stopped, "Operator stop");
}

void SimulationController::reset() {
    if (emergencyLatched_) {
        eventLog_.add(simulationTime_, LogLevel::Safety, "Safety", "Reset blocked: release emergency stop first");
        return;
    }
    shearer_.reset();
    conveyor_.reset();
    for (auto& support : supports_) support.reset();
    std::fill(supportTriggered_.begin(), supportTriggered_.end(), false);
    coalPieces_.clear();
    spawnAccumulator_ = 0.0F;
    lightingFailed_ = false;
    previousState_ = state_;
    state_ = SimulationState::Stopped;
    resumeState_ = SimulationState::Ready;
    stateEnteredAt_ = simulationTime_;
    transitionReason_ = "Full deterministic reset";
    eventLog_.add(simulationTime_, LogLevel::Info, "System", "All equipment and faults reset");
}

void SimulationController::emergencyStop() {
    if (state_ == SimulationState::EmergencyStopped) return;
    emergencyLatched_ = true;
    shearer_.stop();
    conveyor_.stop();
    transitionTo(SimulationState::EmergencyStopped, "Emergency stop button pressed");
    eventLog_.add(simulationTime_, LogLevel::Safety, "Safety", "ALL MOTION STOPPED; release and reset required");
}

void SimulationController::releaseEmergencyStop() {
    if (!emergencyLatched_) return;
    emergencyLatched_ = false;
    eventLog_.add(simulationTime_, LogLevel::Safety, "Safety", "Emergency stop released; system remains stopped");
    transitionTo(SimulationState::Stopped, "Emergency latch released");
}

void SimulationController::injectFault(FaultType type, int supportIndex) {
    switch (type) {
    case FaultType::ShearerOverheat:
        shearer_.setOverheated(true);
        transitionTo(SimulationState::Fault, "Shearer overtemperature trip");
        break;
    case FaultType::ConveyorJam:
        conveyor_.setJammed(true);
        shearer_.stop();
        transitionTo(SimulationState::Fault, "Conveyor jam interlock");
        break;
    case FaultType::SupportLowPressure:
        if (!supports_.empty()) {
            supportIndex = std::clamp(supportIndex, 0, static_cast<int>(supports_.size()) - 1);
            supports_[static_cast<std::size_t>(supportIndex)].setLowPressure(true);
            if (state_ != SimulationState::Stopped) transitionTo(SimulationState::Warning, "Support pressure below threshold");
        }
        break;
    case FaultType::LightingFailure:
        lightingFailed_ = true;
        if (state_ != SimulationState::Stopped) transitionTo(SimulationState::Warning, "Work-face lighting circuit failure");
        break;
    }
    eventLog_.add(simulationTime_, LogLevel::Error, toString(type), "Fault injected for teaching demonstration");
}

void SimulationController::clearFault(FaultType type, int supportIndex) {
    switch (type) {
    case FaultType::ShearerOverheat: shearer_.setOverheated(false); shearer_.stop(); break;
    case FaultType::ConveyorJam: conveyor_.setJammed(false); break;
    case FaultType::SupportLowPressure:
        if (!supports_.empty()) {
            supportIndex = std::clamp(supportIndex, 0, static_cast<int>(supports_.size()) - 1);
            supports_[static_cast<std::size_t>(supportIndex)].setLowPressure(false);
        }
        break;
    case FaultType::LightingFailure: lightingFailed_ = false; break;
    }
    eventLog_.add(simulationTime_, LogLevel::Info, toString(type), "Fault cleared; reset/start sequence required");
    if ((state_ == SimulationState::Fault || state_ == SimulationState::Warning) && !hasCriticalFault()) {
        shearer_.stop();
        conveyor_.stop();
        transitionTo(SimulationState::Stopped, "Fault cleared safely");
    }
}

void SimulationController::clearAllFaults() {
    shearer_.setOverheated(false);
    conveyor_.setJammed(false);
    lightingFailed_ = false;
    for (auto& support : supports_) support.setLowPressure(false);
    if ((state_ == SimulationState::Fault || state_ == SimulationState::Warning) && !emergencyLatched_) {
        shearer_.stop();
        conveyor_.stop();
        transitionTo(SimulationState::Stopped, "All faults cleared safely");
    }
    eventLog_.add(simulationTime_, LogLevel::Info, "System", "All injected faults cleared");
}

void SimulationController::setTimeScale(float scale) { timeScale_ = std::clamp(scale, 0.1F, 4.0F); }

bool SimulationController::hasCriticalFault() const { return shearer_.overheated() || conveyor_.jammed(); }

void SimulationController::update(float realDeltaTime) {
    if (state_ == SimulationState::Paused || state_ == SimulationState::EmergencyStopped) return;
    const float dt = std::clamp(realDeltaTime, 0.0F, 0.1F) * timeScale_;
    simulationTime_ += dt;

    if (state_ == SimulationState::Initializing && simulationTime_ - stateEnteredAt_ >= 0.65) {
        transitionTo(SimulationState::Ready, "Self-check completed");
    }
    if (state_ == SimulationState::StartingConveyor) {
        conveyor_.update(dt, static_cast<int>(coalPieces_.size()));
        if (conveyor_.isRunning()) {
            shearer_.start();
            transitionTo(SimulationState::CuttingForward, "Conveyor at operating speed");
        }
    }

    const bool cutting = state_ == SimulationState::CuttingForward ||
                         state_ == SimulationState::CuttingBackward || state_ == SimulationState::EndTransition;
    if (cutting) {
        shearer_.update(dt, state_ != SimulationState::EndTransition);
        conveyor_.update(dt, static_cast<int>(coalPieces_.size()));
        for (auto& support : supports_) support.update(dt);
        triggerSupports();
        updateCoal(dt);
        if (shearer_.consumedEndpointEvent()) {
            std::fill(supportTriggered_.begin(), supportTriggered_.end(), false);
            transitionTo(SimulationState::EndTransition, "Shearer reached face endpoint");
        }
        if (state_ == SimulationState::EndTransition && simulationTime_ - stateEnteredAt_ >= 0.8) {
            shearer_.resume();
            transitionTo(shearer_.direction() > 0 ? SimulationState::CuttingForward
                                                  : SimulationState::CuttingBackward,
                         "Endpoint turnaround complete");
        }
    } else if (state_ != SimulationState::StartingConveyor) {
        shearer_.update(dt, false);
        conveyor_.update(dt, static_cast<int>(coalPieces_.size()));
    }
}

void SimulationController::triggerSupports() {
    for (std::size_t i = 0; i < supports_.size(); ++i) {
        if (supportTriggered_[i]) continue;
        const float delta = shearer_.position() - supports_[i].x();
        const bool passed = shearer_.direction() > 0 ? delta > 1.8F : delta < -1.8F;
        if (passed) {
            supports_[i].triggerAfterPass();
            supportTriggered_[i] = true;
        }
    }
}

void SimulationController::updateCoal(float dt) {
    if (shearer_.isCutting() && conveyor_.isRunning()) {
        spawnAccumulator_ += dt * 10.0F;
        while (spawnAccumulator_ >= 1.0F && static_cast<int>(coalPieces_.size()) < config_.maxCoalPieces) {
            spawnAccumulator_ -= 1.0F;
            const float seed = static_cast<float>((coalPieces_.size() * 37U) % 101U) / 100.0F;
            CoalPiece piece;
            piece.position = {shearer_.position() + (seed - 0.5F) * 2.2F, 1.2F + seed,
                              layout::coalFaceSurfaceZ + 0.45F};
            piece.velocity = {-conveyor_.speed() * 4.8F, 0.8F + seed, 2.0F};
            piece.size = 0.10F + seed * 0.16F;
            piece.lifetime = 8.0F + seed * 3.0F;
            coalPieces_.push_back(piece);
        }
    }
    for (auto& piece : coalPieces_) {
        piece.age += dt;
        if (piece.position.y > 0.42F) {
            piece.velocity.y -= 5.5F * dt;
            piece.position += piece.velocity * dt;
            if (piece.position.y < 0.42F) { piece.position.y = 0.42F; piece.velocity.y = 0.0F; }
        } else {
            piece.position.x -= conveyor_.speed() * 4.0F * dt;
            piece.position.z += (layout::conveyorCenterZ - piece.position.z) * std::min(1.0F, dt * 5.0F);
        }
    }
    coalPieces_.erase(std::remove_if(coalPieces_.begin(), coalPieces_.end(),
                                     [&](const CoalPiece& p) {
                                         return p.age >= p.lifetime || p.position.x < -config_.faceLength * 0.55F;
                                     }),
                      coalPieces_.end());
}

const char* toString(SimulationState state) {
    switch (state) {
    case SimulationState::Stopped: return "Stopped";
    case SimulationState::Initializing: return "Initializing";
    case SimulationState::Ready: return "Ready";
    case SimulationState::StartingConveyor: return "Starting conveyor";
    case SimulationState::CuttingForward: return "Cutting forward";
    case SimulationState::EndTransition: return "Endpoint transition";
    case SimulationState::CuttingBackward: return "Cutting backward";
    case SimulationState::Paused: return "Paused";
    case SimulationState::Warning: return "Warning";
    case SimulationState::Fault: return "Fault";
    case SimulationState::EmergencyStopped: return "EMERGENCY STOPPED";
    }
    return "Unknown";
}

const char* toString(FaultType type) {
    switch (type) {
    case FaultType::ShearerOverheat: return "Shearer overheat";
    case FaultType::ConveyorJam: return "Conveyor jam";
    case FaultType::SupportLowPressure: return "Support pressure";
    case FaultType::LightingFailure: return "Lighting circuit";
    }
    return "Unknown fault";
}

} // namespace mine
