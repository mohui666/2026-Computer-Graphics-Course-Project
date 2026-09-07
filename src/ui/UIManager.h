#pragma once

#include <string>

struct GLFWwindow;

namespace mine {
class Camera;
class SimulationController;
struct RenderConfig;
struct RenderStats;

class UIManager {
public:
    void initialize(GLFWwindow* window);
    void shutdown();
    void beginFrame();
    void update(SimulationController& simulation, Camera& camera);
    void primaryAction(SimulationController& simulation);
    void stopFollowing() { followShearer_ = false; }
    void render(SimulationController& simulation, RenderConfig& renderConfig, Camera& camera,
                int& selectedId, const RenderStats& stats, float fps, float frameMs);
    void endFrame();
    void toggleHelp() { showHelp_ = !showHelp_; }

private:
    void restart(SimulationController& simulation);
    void drawEquipment(SimulationController& simulation, Camera& camera, int& selectedId);
    void drawFaults(SimulationController& simulation);
    void drawRenderSettings(RenderConfig& config);
    void drawLog(SimulationController& simulation);
    void drawHelp();

    bool pendingStart_ = false;
    bool followShearer_ = true;
    bool showHelp_ = false;
    bool autoScroll_ = true;
    int supportFaultIndex_ = 0;
    std::string glslVersion_ = "#version 330";
};

} // namespace mine
