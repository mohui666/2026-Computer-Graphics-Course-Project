#include "ui/UIManager.h"

#include "core/Config.h"
#include "graphics/Camera.h"
#include "graphics/Renderer.h"
#include "simulation/SimulationController.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <algorithm>
#include <cstdio>

namespace mine {
namespace {
void coloredState(const char* label, bool danger = false, bool warning = false) {
    const ImVec4 color = danger ? ImVec4(1.0F,0.18F,0.08F,1.0F)
                                : warning ? ImVec4(1.0F,0.68F,0.08F,1.0F)
                                          : ImVec4(0.25F,0.92F,0.52F,1.0F);
    ImGui::TextColored(color, "%s", label);
}
}

void UIManager::initialize(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0F;
    style.FrameRounding = 3.0F;
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.12F, 0.24F, 0.27F, 1.0F);
    style.Colors[ImGuiCol_Button] = ImVec4(0.12F, 0.31F, 0.35F, 1.0F);
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glslVersion_.c_str());
}

void UIManager::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UIManager::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UIManager::render(SimulationController& simulation, RenderConfig& renderConfig, Camera& camera,
                       int& selectedId, const RenderStats& stats, float fps, float frameMs) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Windows")) {
            ImGui::MenuItem("Simulation control", nullptr, &showControls_);
            ImGui::MenuItem("Equipment", nullptr, &showEquipment_);
            ImGui::MenuItem("Fault injection", nullptr, &showFaults_);
            ImGui::MenuItem("Rendering", nullptr, &showRender_);
            ImGui::MenuItem("Event log", nullptr, &showLog_);
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("Help / F1")) showHelp_ = true;
        ImGui::Separator();
        ImGui::Text("FPS %.1f | %.2f ms | Draws %d | Coal %zu", fps, frameMs, stats.drawCalls,
                    simulation.coalPieces().size());
        ImGui::EndMainMenuBar();
    }
    if (showControls_) drawControls(simulation);
    if (showEquipment_) drawEquipment(simulation, camera, selectedId);
    if (showFaults_) drawFaults(simulation);
    if (showRender_) drawRenderSettings(renderConfig, camera, simulation);
    if (showLog_) drawLog(simulation);
    if (showHelp_) drawHelp();
}

void UIManager::endFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UIManager::drawControls(SimulationController& simulation) {
    const float panelX = std::max(12.0F, ImGui::GetIO().DisplaySize.x - 342.0F);
    ImGui::SetNextWindowPos({panelX, 32}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({330, 238}, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Longwall Control Station", &showControls_)) { ImGui::End(); return; }
    const auto state = simulation.state();
    ImGui::TextUnformatted("SYSTEM STATE"); ImGui::SameLine();
    coloredState(toString(state), state == SimulationState::Fault || state == SimulationState::EmergencyStopped,
                 state == SimulationState::Warning || state == SimulationState::Paused);
    ImGui::TextWrapped("Transition: %s", simulation.transitionReason().c_str());
    ImGui::Text("Simulation time: %.1f s", simulation.simulationTime());
    ImGui::Separator();
    if (ImGui::Button("Initialize")) simulation.initialize(); ImGui::SameLine();
    if (ImGui::Button("Start")) simulation.start(); ImGui::SameLine();
    if (ImGui::Button("Pause")) simulation.pause(); ImGui::SameLine();
    if (ImGui::Button("Resume")) simulation.resume();
    if (ImGui::Button("Stop")) simulation.stop(); ImGui::SameLine();
    if (ImGui::Button("Reset")) simulation.reset();
    ImGui::Separator();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.72F,0.04F,0.025F,1));
    if (ImGui::Button("EMERGENCY STOP", {190, 34})) simulation.emergencyStop();
    ImGui::PopStyleColor(); ImGui::SameLine();
    if (ImGui::Button("Release latch", {115,34})) simulation.releaseEmergencyStop();
    float scale = simulation.timeScale();
    if (ImGui::SliderFloat("Time scale", &scale, 0.1F, 4.0F, "%.1fx")) simulation.setTimeScale(scale);
    ImGui::Text("Interlock: %s | E-stop latch: %s", simulation.conveyor().isRunning() ? "conveyor ready" : "waiting",
                simulation.emergencyLatched() ? "LOCKED" : "released");
    ImGui::End();
}

void UIManager::drawEquipment(SimulationController& simulation, Camera& camera, int& selectedId) {
    const float panelX = std::max(12.0F, ImGui::GetIO().DisplaySize.x - 342.0F);
    ImGui::SetNextWindowPos({panelX, 282}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({330, 370}, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Equipment and telemetry", &showEquipment_)) { ImGui::End(); return; }
    if (ImGui::BeginListBox("##equipment", {-1, 130})) {
        if (ImGui::Selectable("[01] Double-drum shearer", selectedId == 1)) selectedId = 1;
        if (ImGui::Selectable("[02] Scraper conveyor", selectedId == 2)) selectedId = 2;
        if (ImGui::Selectable("[03] Work-face lighting", selectedId == 3)) selectedId = 3;
        for (const auto& support : simulation.supports()) {
            if (ImGui::Selectable(support.name().c_str(), selectedId == support.id())) selectedId = support.id();
        }
        ImGui::EndListBox();
    }
    if (ImGui::Button("Focus selected")) {
        if (selectedId == 1) camera.applyPreset(CameraPreset::Shearer, simulation.shearer().position());
        else if (selectedId == 2) camera.applyPreset(CameraPreset::Conveyor);
        else if (selectedId >= 101 && selectedId < 101 + static_cast<int>(simulation.supports().size())) {
            camera.focus({simulation.supports()[static_cast<std::size_t>(selectedId - 101)].x(),
                          2.8F, layout::supportCenterZ});
        }
    }
    ImGui::Separator();
    if (selectedId == 1) {
        const auto& s = simulation.shearer();
        ImGui::Text("Shearer: %s", toString(s.status()));
        ImGui::Text("Position %+.2f m  Direction %s", s.position(), s.direction() > 0 ? "forward" : "backward");
        ImGui::Text("Speed %.2f m/s  Load %.1f %%", s.speed(), s.load());
        ImGui::ProgressBar(s.load() / 100.0F, {-1,0});
        ImGui::Text("Temperature %.1f C", s.temperature());
        ImGui::ProgressBar(std::clamp(s.temperature() / 110.0F, 0.0F, 1.0F), {-1,0});
    } else if (selectedId == 2) {
        auto& c = simulation.conveyor();
        ImGui::Text("Conveyor: %s", toString(c.status()));
        ImGui::Text("Chain speed %.2f m/s  Load %.1f %%", c.speed(), c.load());
        float scale = c.speedScale();
        if (ImGui::SliderFloat("Speed command", &scale, 0.2F, 2.0F, "%.1fx")) c.setSpeedScale(scale);
        ImGui::ProgressBar(c.load() / 100.0F, {-1,0});
    } else if (selectedId == 3) {
        ImGui::Text("Lighting circuit: %s", simulation.lightingFailed() ? "FAILED" : "NORMAL");
        ImGui::TextWrapped("Six warm work lamps and camera headlamp illuminate the face independently.");
    } else if (selectedId >= 101 && selectedId < 101 + static_cast<int>(simulation.supports().size())) {
        const auto& s = simulation.supports()[static_cast<std::size_t>(selectedId - 101)];
        ImGui::Text("%s", s.name().c_str());
        ImGui::Text("Stage: %s", toString(s.stage()));
        ImGui::Text("Pressure: %.1f MPa", s.pressure());
        ImGui::ProgressBar(s.pressure() / 40.0F, {-1,0});
        ImGui::Text("Action progress: %.0f %%", s.progress() * 100.0F);
    } else ImGui::TextWrapped("Click equipment in the scene or select it above.");
    ImGui::End();
}

void UIManager::drawFaults(SimulationController& simulation) {
    ImGui::SetNextWindowPos({380, 32}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({325, 260}, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Fault demonstration", &showFaults_)) { ImGui::End(); return; }
    ImGui::TextWrapped("Faults alter equipment and safety interlocks; they are not text-only alarms.");
    if (ImGui::Button("Inject shearer overheat")) simulation.injectFault(FaultType::ShearerOverheat);
    ImGui::SameLine(); if (ImGui::SmallButton("Clear##heat")) simulation.clearFault(FaultType::ShearerOverheat);
    if (ImGui::Button("Inject conveyor jam")) simulation.injectFault(FaultType::ConveyorJam);
    ImGui::SameLine(); if (ImGui::SmallButton("Clear##jam")) simulation.clearFault(FaultType::ConveyorJam);
    const int maxSupport = std::max(0, static_cast<int>(simulation.supports().size()) - 1);
    ImGui::SliderInt("Support index", &supportFaultIndex_, 0, maxSupport, "#%d");
    if (ImGui::Button("Low support pressure")) simulation.injectFault(FaultType::SupportLowPressure, supportFaultIndex_);
    ImGui::SameLine(); if (ImGui::SmallButton("Clear##support")) simulation.clearFault(FaultType::SupportLowPressure, supportFaultIndex_);
    if (ImGui::Button("Fail work-face lights")) simulation.injectFault(FaultType::LightingFailure);
    ImGui::SameLine(); if (ImGui::SmallButton("Clear##lights")) simulation.clearFault(FaultType::LightingFailure);
    if (ImGui::Button("Clear all injected faults", {-1,0})) simulation.clearAllFaults();
    ImGui::End();
}

void UIManager::drawRenderSettings(RenderConfig& config, Camera& camera,
                                   const SimulationController& simulation) {
    ImGui::SetNextWindowPos({715, 32}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({315, 345}, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Rendering and cameras", &showRender_)) { ImGui::End(); return; }
    ImGui::Checkbox("Distance fog", &config.fogEnabled);
    ImGui::SliderFloat("Fog density", &config.fogDensity, 0.0F, 0.075F, "%.3f");
    ImGui::Checkbox("Work lights", &config.workLights); ImGui::SameLine();
    ImGui::Checkbox("Headlamp", &config.headlamp);
    ImGui::SeparatorText("Post processing");
    ImGui::Checkbox("HDR bloom", &config.bloomEnabled); ImGui::SameLine();
    ImGui::Checkbox("FXAA", &config.fxaaEnabled);
    ImGui::Checkbox("Contact shading (SSAO)", &config.ssaoEnabled);
    ImGui::SliderFloat("Exposure (EV)", &config.exposureEv, -1.0F, 1.2F, "%+.2f");
    ImGui::SliderFloat("Bloom strength", &config.bloomStrength, 0.0F, 0.30F, "%.2f");
    ImGui::SliderFloat("Vignette", &config.vignetteStrength, 0.0F, 0.35F, "%.2f");
    ImGui::SliderFloat("Contact shading", &config.ssaoStrength, 0.0F, 0.65F, "%.2f");
    ImGui::Checkbox("Wireframe", &config.wireframe); ImGui::SameLine();
    ImGui::Checkbox("Axes", &config.showAxes); ImGui::SameLine();
    ImGui::Checkbox("Bounds", &config.showBounds);
    ImGui::Checkbox("VSync", &config.vsync);
    if (ImGui::Button("Overview [1]")) camera.applyPreset(CameraPreset::Overview); ImGui::SameLine();
    if (ImGui::Button("Shearer [2]")) camera.applyPreset(CameraPreset::Shearer, simulation.shearer().position());
    if (ImGui::Button("Supports [3]")) camera.applyPreset(CameraPreset::Supports); ImGui::SameLine();
    if (ImGui::Button("Conveyor [4]")) camera.applyPreset(CameraPreset::Conveyor, simulation.shearer().position());
    if (ImGui::Button("Entrance [5]")) camera.applyPreset(CameraPreset::Entrance); ImGui::SameLine();
    if (ImGui::Button("Top [6]")) camera.applyPreset(CameraPreset::Top);
    ImGui::End();
}

void UIManager::drawLog(SimulationController& simulation) {
    ImGui::SetNextWindowPos({380, 560}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({760, 260}, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Event log", &showLog_)) { ImGui::End(); return; }
    ImGui::Checkbox("Info", &logInfo_); ImGui::SameLine(); ImGui::Checkbox("Warnings", &logWarning_);
    ImGui::SameLine(); ImGui::Checkbox("Errors/Safety", &logError_); ImGui::SameLine();
    ImGui::Checkbox("Auto scroll", &autoScroll_); ImGui::SameLine();
    if (ImGui::Button("Clear")) simulation.eventLog().clear();
    ImGui::BeginChild("log-scroller", {0,0}, ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& entry : simulation.eventLog().entries()) {
        if ((entry.level == LogLevel::Info && !logInfo_) ||
            (entry.level == LogLevel::Warning && !logWarning_) ||
            ((entry.level == LogLevel::Error || entry.level == LogLevel::Safety) && !logError_)) continue;
        const ImVec4 color = entry.level == LogLevel::Info ? ImVec4(0.72F,0.8F,0.82F,1)
                             : entry.level == LogLevel::Warning ? ImVec4(1,0.72F,0.15F,1)
                             : ImVec4(1,0.25F,0.12F,1);
        ImGui::TextColored(color, "[%07.2f] %-6s %-18s %s", entry.time, toString(entry.level),
                           entry.source.c_str(), entry.message.c_str());
    }
    if (autoScroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 20.0F) ImGui::SetScrollHereY(1.0F);
    ImGui::EndChild();
    ImGui::End();
}

void UIManager::drawHelp() {
    ImGui::SetNextWindowSize({620, 480}, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("MineLongwallSimulation Help", &showHelp_)) { ImGui::End(); return; }
    ImGui::TextWrapped("Teaching visualization only. This program is not mine-control software and is not suitable for safety certification or production decisions.");
    ImGui::Separator();
    ImGui::TextUnformatted("Navigation");
    ImGui::BulletText("W/A/S/D: move, Q/E: descend/ascend, Shift: faster");
    ImGui::BulletText("Hold right mouse: look around; left click: select equipment");
    ImGui::BulletText("1-6: overview, shearer, supports, conveyor, entrance, top views");
    ImGui::BulletText("R: reset camera, F: focus selected, F12: screenshot, F1: help, Esc: quit");
    ImGui::Separator();
    ImGui::TextUnformatted("Recommended demonstration flow");
    ImGui::BulletText("Initialize, wait for Ready, then Start (conveyor interlock runs first).");
    ImGui::BulletText("Observe deterministic shearer telemetry, coal transport and sequential supports.");
    ImGui::BulletText("Inject a jam or overheat, clear it, then initialize/start again.");
    ImGui::BulletText("Press emergency stop; verify Reset is blocked until the latch is released.");
    ImGui::Separator();
    ImGui::TextWrapped("Debug views: Wireframe shows mesh topology, Axes shows world directions, and Bounds shows the selected device's picking AABBs.");
    ImGui::End();
}

} // namespace mine
