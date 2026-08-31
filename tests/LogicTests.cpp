#include "core/Config.h"
#include "equipment/HydraulicSupport.h"
#include "equipment/ScraperConveyor.h"
#include "equipment/Shearer.h"
#include "scene/Transform.h"
#include "simulation/SimulationController.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

namespace {
int failures = 0;
int checks = 0;

void check(bool condition, const std::string& name) {
    ++checks;
    if (!condition) {
        ++failures;
        std::cerr << "[FAIL] " << name << '\n';
    } else {
        std::cout << "[PASS] " << name << '\n';
    }
}

bool near(float lhs, float rhs, float epsilon = 0.01F) { return std::abs(lhs - rhs) < epsilon; }

void advanceToReady(mine::SimulationController& simulation) {
    simulation.initialize();
    for (int i = 0; i < 10; ++i) simulation.update(0.1F);
}
}

int main() {
    {
        mine::Transform parent;
        mine::Transform child;
        parent.setPosition({10.0F, 2.0F, -1.0F});
        child.setPosition({3.0F, 1.0F, 4.0F});
        child.setParent(&parent);
        const auto world = child.worldPosition();
        check(near(world.x, 13.0F) && near(world.y, 3.0F) && near(world.z, 3.0F),
              "Transform composes parent and child matrices");
    }
    {
        mine::SimulationController simulation;
        check(simulation.canTransition(mine::SimulationState::Initializing), "Stopped -> Initializing is legal");
        check(!simulation.canTransition(mine::SimulationState::CuttingForward), "Stopped -> Cutting is rejected");
        advanceToReady(simulation);
        check(simulation.state() == mine::SimulationState::Ready, "Initialization reaches Ready");
        simulation.start();
        for (int i = 0; i < 20; ++i) simulation.update(0.1F);
        check(simulation.state() == mine::SimulationState::CuttingForward, "Interlocked start reaches CuttingForward");
    }
    {
        mine::Shearer shearer(-2.0F, 2.0F, 4.0F);
        check(near(shearer.transform().worldPosition().z, mine::layout::conveyorCenterZ) &&
                  shearer.leftDrumTransform().worldPosition().z < shearer.transform().worldPosition().z,
              "Shearer rides the conveyor while drums reach toward the coal face");
        const float lowerDrumY = std::min(shearer.leftDrumTransform().worldPosition().y,
                                         shearer.rightDrumTransform().worldPosition().y);
        const float upperDrumY = std::max(shearer.leftDrumTransform().worldPosition().y,
                                         shearer.rightDrumTransform().worldPosition().y);
        check(lowerDrumY - mine::layout::shearerCutterEnvelopeRadius > mine::layout::conveyorGuideTopY,
              "Shearer cutter envelope clears the conveyor guides");
        check(upperDrumY + mine::layout::shearerCutterEnvelopeRadius < mine::layout::roofUndersideY,
              "Shearer cutter envelope clears the roof");
        const float drumCenterZ = shearer.leftDrumTransform().worldPosition().z;
        check(drumCenterZ - mine::layout::shearerDrumRadius > mine::layout::coalFaceSurfaceZ &&
                  drumCenterZ - mine::layout::shearerCutterEnvelopeRadius < mine::layout::coalFaceSurfaceZ,
              "Only cutter picks enter the coal face, not the drum body");
        shearer.start();
        for (int i = 0; i < 20; ++i) shearer.update(0.1F, true);
        check(shearer.direction() == -1 && shearer.position() <= 2.0F, "Shearer reverses at endpoint");
        check(shearer.consumedEndpointEvent(), "Shearer emits one endpoint event");
        check(!shearer.consumedEndpointEvent(), "Endpoint event is consumed deterministically");
    }
    {
        mine::SimulationController simulation;
        advanceToReady(simulation);
        simulation.start();
        for (int i = 0; i < 20; ++i) simulation.update(0.1F);
        const float beforePause = simulation.shearer().position();
        const double timeBeforePause = simulation.simulationTime();
        simulation.pause();
        for (int i = 0; i < 20; ++i) simulation.update(0.1F);
        check(near(simulation.shearer().position(), beforePause), "Pause freezes equipment motion");
        check(std::abs(simulation.simulationTime() - timeBeforePause) < 0.001, "Pause freezes simulation clock");
        simulation.resume();
        simulation.update(0.1F);
        check(simulation.state() != mine::SimulationState::Paused, "Resume restores previous state");
    }
    {
        mine::SimulationController simulation;
        advanceToReady(simulation);
        simulation.start();
        for (int i = 0; i < 30; ++i) simulation.update(0.1F);
        simulation.emergencyStop();
        const float position = simulation.shearer().position();
        simulation.update(1.0F);
        check(simulation.state() == mine::SimulationState::EmergencyStopped &&
                  near(simulation.shearer().position(), position) && near(simulation.conveyor().speed(), 0.0F),
              "Emergency stop immediately freezes all motion");
        simulation.reset();
        check(simulation.state() == mine::SimulationState::EmergencyStopped,
              "Reset is blocked while emergency latch is active");
        simulation.releaseEmergencyStop();
        simulation.reset();
        check(simulation.state() == mine::SimulationState::Stopped && near(simulation.shearer().position(), -32.4F, 0.1F),
              "Release then reset restores deterministic initial state");
    }
    {
        mine::HydraulicSupport support(1, 0.0F, 0.2F, 0.2F);
        support.triggerAfterPass();
        check(support.stage() == mine::SupportStage::Waiting, "Support begins with delayed waiting stage");
        support.update(0.21F);
        check(support.stage() == mine::SupportStage::Lowering, "Support sequence enters lowering");
        support.update(0.21F);
        check(support.stage() == mine::SupportStage::Advancing, "Support sequence enters advancing");
        support.update(0.21F);
        check(support.stage() == mine::SupportStage::Raising, "Support sequence enters raising");
        support.update(0.21F);
        check(support.stage() == mine::SupportStage::Normal, "Support sequence returns to normal");
    }
    {
        mine::HydraulicSupport support(2, 0.0F, 0.1F, 0.4F);
        support.triggerAfterPass();
        auto previous = support.visualPose();
        float maxHeightStep = 0.0F;
        float maxAdvanceStep = 0.0F;
        float minimumAdvance = 0.0F;
        for (int i = 0; i < 150; ++i) {
            support.update(0.01F);
            const auto pose = support.visualPose();
            maxHeightStep = std::max(maxHeightStep, std::abs(pose.heightScale - previous.heightScale));
            maxAdvanceStep = std::max(maxAdvanceStep, std::abs(pose.advanceOffset - previous.advanceOffset));
            minimumAdvance = std::min(minimumAdvance, pose.advanceOffset);
            previous = pose;
        }
        check(maxHeightStep < 0.02F && maxAdvanceStep < 0.02F &&
                  near(previous.heightScale, 1.0F) && near(previous.advanceOffset, 0.0F),
              "Support visual pose stays continuous through the complete cycle");
        const float supportCoalEdge = mine::layout::supportCenterZ + minimumAdvance -
                                      mine::layout::supportBaseHalfDepth;
        const float conveyorWalkwayEdge = mine::layout::conveyorCenterZ + mine::layout::conveyorHalfWidth;
        check(minimumAdvance >= -mine::layout::supportAdvanceDistance - 0.001F &&
                  supportCoalEdge - conveyorWalkwayEdge > 0.08F,
              "Support advance preserves clearance from the conveyor");
    }
    {
        mine::ScraperConveyor conveyor(1.0F);
        auto positions = conveyor.scraperPositions(72.0F);
        bool insideEnds = !positions.empty();
        for (float position : positions) insideEnds = insideEnds && std::abs(position) + 0.11F < 36.0F;
        check(insideEnds, "Conveyor scrapers stay inside the head and tail assemblies");

        conveyor.start();
        conveyor.update(0.5F, 0);
        conveyor.update(2.69F, 0);
        const auto beforeWrap = conveyor.scraperPositions(72.0F);
        conveyor.update(0.02F, 0);
        const auto afterWrap = conveyor.scraperPositions(72.0F);
        const float cycleLength = static_cast<float>(beforeWrap.size()) * mine::layout::scraperSpacing;
        float worstNearestDistance = 0.0F;
        for (float before : beforeWrap) {
            float nearest = 1000.0F;
            for (float after : afterWrap) {
                const float direct = std::abs(before - after);
                nearest = std::min(nearest, std::min(direct, cycleLength - direct));
            }
            worstNearestDistance = std::max(worstNearestDistance, nearest);
        }
        check(worstNearestDistance < 0.03F, "Conveyor scraper loop wraps without a visible jump");
    }
    {
        mine::SimulationController simulation;
        advanceToReady(simulation);
        simulation.start();
        simulation.injectFault(mine::FaultType::ConveyorJam);
        check(simulation.state() == mine::SimulationState::Fault && simulation.conveyor().jammed(),
              "Conveyor jam trips the simulation and changes equipment behavior");
        simulation.clearFault(mine::FaultType::ConveyorJam);
        check(simulation.state() == mine::SimulationState::Stopped && !simulation.conveyor().jammed(),
              "Clearing critical fault returns to safe stopped state");
    }
    {
        mine::SimulationConfig config;
        config.supportCount = 1;
        config.faceLength = 999.0F;
        config.shearerSpeed = -5.0F;
        config.maxCoalPieces = 9999;
        config.sanitize();
        check(config.supportCount == 12 && near(config.faceLength, 160.0F) && near(config.shearerSpeed, 0.5F) &&
                  config.maxCoalPieces == 1000,
              "Configuration clamps invalid boundary values");
    }

    std::cout << "\n" << checks - failures << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
