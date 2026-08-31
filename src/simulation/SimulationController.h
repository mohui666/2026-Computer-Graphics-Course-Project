#pragma once

#include "core/Config.h"
#include "equipment/HydraulicSupport.h"
#include "equipment/ScraperConveyor.h"
#include "equipment/Shearer.h"
#include "simulation/EventLog.h"

#include <string>
#include <vector>

namespace mine {

enum class SimulationState {
    Stopped,
    Initializing,
    Ready,
    StartingConveyor,
    CuttingForward,
    EndTransition,
    CuttingBackward,
    Paused,
    Warning,
    Fault,
    EmergencyStopped
};

enum class FaultType { ShearerOverheat, ConveyorJam, SupportLowPressure, LightingFailure };

struct CoalPiece {
    glm::vec3 position{0.0F};
    glm::vec3 velocity{0.0F};
    float age = 0.0F;
    float lifetime = 9.0F;
    float size = 0.16F;
};

class SimulationController {
public:
    explicit SimulationController(SimulationConfig config = {});

    void initialize();
    void start();
    void pause();
    void resume();
    void stop();
    void reset();
    void emergencyStop();
    void releaseEmergencyStop();
    void injectFault(FaultType type, int supportIndex = 0);
    void clearFault(FaultType type, int supportIndex = 0);
    void clearAllFaults();
    void update(float realDeltaTime);

    [[nodiscard]] bool canTransition(SimulationState to) const;
    [[nodiscard]] SimulationState state() const { return state_; }
    [[nodiscard]] SimulationState previousState() const { return previousState_; }
    [[nodiscard]] double stateEnteredAt() const { return stateEnteredAt_; }
    [[nodiscard]] const std::string& transitionReason() const { return transitionReason_; }
    [[nodiscard]] double simulationTime() const { return simulationTime_; }
    [[nodiscard]] float timeScale() const { return timeScale_; }
    void setTimeScale(float scale);
    [[nodiscard]] bool lightingFailed() const { return lightingFailed_; }
    [[nodiscard]] bool emergencyLatched() const { return emergencyLatched_; }

    [[nodiscard]] Shearer& shearer() { return shearer_; }
    [[nodiscard]] const Shearer& shearer() const { return shearer_; }
    [[nodiscard]] ScraperConveyor& conveyor() { return conveyor_; }
    [[nodiscard]] const ScraperConveyor& conveyor() const { return conveyor_; }
    [[nodiscard]] std::vector<HydraulicSupport>& supports() { return supports_; }
    [[nodiscard]] const std::vector<HydraulicSupport>& supports() const { return supports_; }
    [[nodiscard]] const std::vector<CoalPiece>& coalPieces() const { return coalPieces_; }
    [[nodiscard]] EventLog& eventLog() { return eventLog_; }
    [[nodiscard]] const EventLog& eventLog() const { return eventLog_; }
    [[nodiscard]] const SimulationConfig& config() const { return config_; }

private:
    void transitionTo(SimulationState next, std::string reason);
    void updateCoal(float dt);
    void triggerSupports();
    bool hasCriticalFault() const;

    SimulationConfig config_;
    Shearer shearer_;
    ScraperConveyor conveyor_;
    std::vector<HydraulicSupport> supports_;
    std::vector<CoalPiece> coalPieces_;
    EventLog eventLog_;
    SimulationState state_ = SimulationState::Stopped;
    SimulationState previousState_ = SimulationState::Stopped;
    SimulationState resumeState_ = SimulationState::Ready;
    double simulationTime_ = 0.0;
    double stateEnteredAt_ = 0.0;
    std::string transitionReason_ = "Application created";
    float timeScale_ = 1.0F;
    float spawnAccumulator_ = 0.0F;
    bool lightingFailed_ = false;
    bool emergencyLatched_ = false;
    std::vector<bool> supportTriggered_;
};

const char* toString(SimulationState state);
const char* toString(FaultType type);

} // namespace mine
