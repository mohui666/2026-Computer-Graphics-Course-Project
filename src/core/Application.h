#pragma once

#include "core/Config.h"
#include "graphics/Camera.h"
#include "particles/ParticleSystem.h"
#include "simulation/SimulationController.h"
#include "ui/UIManager.h"

#include <memory>
#include <string>

struct GLFWwindow;

namespace mine {
class Renderer;

class Application {
public:
    explicit Application(bool captureMode = false, CameraPreset capturePreset = CameraPreset::Shearer,
                         float captureTime = 4.0F, std::string captureOutput = "screenshots/latest.bmp",
                         bool captureUi = false);
    ~Application();
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    int run();

private:
    void initializeWindow();
    void processInput(float dt);
    void shutdown();
    bool pressedOnce(int key);

    GLFWwindow* window_ = nullptr;
    std::unique_ptr<Renderer> renderer_;
    SimulationController simulation_;
    ParticleSystem particles_;
    Camera camera_;
    UIManager ui_;
    RenderConfig renderConfig_;
    int selectedId_ = -1;
    bool uiInitialized_ = false;
    bool mouseLookStarted_ = false;
    bool leftMouseWasDown_ = false;
    bool screenshotRequested_ = false;
    bool captureMode_ = false;
    bool showUi_ = true;
    bool captureUi_ = false;
    float captureTime_ = 4.0F;
    std::string captureOutput_;
    double particleSimulationTime_ = 0;
    CameraPreset capturePreset_ = CameraPreset::Shearer;
    int frameCount_ = 0;
    double lastMouseX_ = 0.0;
    double lastMouseY_ = 0.0;
    bool previousKeys_[512]{};
};

} // namespace mine
