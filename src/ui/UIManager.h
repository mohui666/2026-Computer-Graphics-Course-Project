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
    void render(SimulationController& simulation, RenderConfig& renderConfig, Camera& camera,
                int& selectedId, const RenderStats& stats, float fps, float frameMs);
    void endFrame();
    void toggleHelp() { showHelp_ = !showHelp_; }

private:
    void drawControls(SimulationController& simulation);
    void drawEquipment(SimulationController& simulation, Camera& camera, int& selectedId);
    void drawFaults(SimulationController& simulation);
    void drawRenderSettings(RenderConfig& config, Camera& camera, const SimulationController& simulation);
    void drawLog(SimulationController& simulation);
    void drawHelp();

    bool showControls_ = true;
    bool showEquipment_ = false;
    bool showFaults_ = false;
    bool showRender_ = false;
    bool showLog_ = false;
    bool showHelp_ = false;
    bool autoScroll_ = true;
    bool logInfo_ = true;
    bool logWarning_ = true;
    bool logError_ = true;
    int supportFaultIndex_ = 0;
    std::string glslVersion_ = "#version 330";
};

} // namespace mine
