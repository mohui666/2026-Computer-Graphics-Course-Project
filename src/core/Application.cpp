#include "core/Application.h"

#include "graphics/Renderer.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace mine {
namespace {
void errorCallback(int code, const char* description) {
    std::cerr << "GLFW error " << code << ": " << description << '\n';
}
}

Application::Application(bool captureMode, CameraPreset capturePreset, float captureTime, std::string captureOutput, bool captureUi)
    : simulation_(SimulationConfig{}), captureMode_(captureMode), captureUi_(captureUi), captureTime_(captureTime),
      captureOutput_(std::move(captureOutput)), capturePreset_(capturePreset) {}
Application::~Application() { shutdown(); }

void Application::initializeWindow() {
    glfwSetErrorCallback(errorCallback);
    if (!glfwInit()) throw std::runtime_error("GLFW initialization failed");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    if (captureMode_) glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    window_ = glfwCreateWindow(1500, 900, "煤矿综采工作面 · OpenGL 仿真演示", nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        throw std::runtime_error("Unable to create an OpenGL 3.3 Core window");
    }
    glfwMakeContextCurrent(window_);
    if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress))) {
        throw std::runtime_error("GLAD failed to load OpenGL functions");
    }
    glfwSwapInterval(renderConfig_.vsync ? 1 : 0);
    glEnable(GL_MULTISAMPLE);

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << '\n';
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << '\n';
    renderer_ = std::make_unique<Renderer>();
    renderer_->initialize(MINE_ASSET_DIR);
    ui_.initialize(window_);
    uiInitialized_ = true;
}

int Application::run() {
    try {
        initializeWindow();
        double previousTime = glfwGetTime();
        bool appliedVsync = renderConfig_.vsync;
        while (!glfwWindowShouldClose(window_)) {
            glfwPollEvents();
            const double now = glfwGetTime();
            const float dt = captureMode_ ? 1.0F/60.0F : std::min(0.1F, static_cast<float>(now - previousTime));
            previousTime = now;

            ui_.beginFrame();
            if (!captureMode_) processInput(dt);
            if (captureMode_ && frameCount_ == 6) ui_.primaryAction(simulation_);
            if (simulation_.state() == SimulationState::Initializing ||
                simulation_.simulationTime() < particleSimulationTime_) particles_.clear();
            simulation_.update(dt);
            particleSimulationTime_ = simulation_.simulationTime();
            ui_.update(simulation_, camera_);
            if (captureMode_) camera_.applyPreset(capturePreset_, simulation_.shearer().position());
            const bool frozen = simulation_.state() == SimulationState::Paused ||
                                simulation_.state() == SimulationState::EmergencyStopped;
            particles_.update(frozen ? 0.0F : dt * simulation_.timeScale(), simulation_.shearer().isCutting(),
                              simulation_.shearer().leftDrumTransform().worldPosition(),
                              simulation_.shearer().rightDrumTransform().worldPosition());

            int width = 1;
            int height = 1;
            glfwGetFramebufferSize(window_, &width, &height);
            renderer_->render(simulation_, camera_, particles_, renderConfig_, std::max(width,1),
                              std::max(height,1), selectedId_);
            const float fps = dt > 0.00001F ? 1.0F / dt : 0.0F;
            if (showUi_ && (!captureMode_ || captureUi_)) ui_.render(simulation_, renderConfig_, camera_, selectedId_, renderer_->stats(), fps, dt * 1000.0F);
            ui_.endFrame();
            ++frameCount_;
            if (captureMode_ && simulation_.simulationTime() >= captureTime_) screenshotRequested_ = true;
            if (screenshotRequested_) {
                const bool saved = renderer_->saveScreenshotBmp(captureMode_ ? captureOutput_ : "screenshots/latest.bmp", std::max(width,1),
                                                                 std::max(height,1));
                simulation_.eventLog().add(simulation_.simulationTime(), saved ? LogLevel::Info : LogLevel::Error,
                                           "Renderer", saved ? "Screenshot saved to screenshots/latest.bmp"
                                                             : "Screenshot save failed");
                screenshotRequested_ = false;
                if (captureMode_) {
                    const GLenum error = glGetError();
                    std::cout << "Capture: " << captureOutput_ << " | simulation=" << simulation_.simulationTime()
                              << " | state=" << toString(simulation_.state())
                              << " | draws=" << renderer_->stats().drawCalls << " | GL error=" << error << '\n';
                    if (!saved || error != GL_NO_ERROR) throw std::runtime_error("Capture failed");
                    glfwSetWindowShouldClose(window_, GLFW_TRUE);
                }
            }
            glfwSwapBuffers(window_);

            if (renderConfig_.vsync != appliedVsync) {
                appliedVsync = renderConfig_.vsync;
                glfwSwapInterval(appliedVsync ? 1 : 0);
            }
        }
        shutdown();
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "Fatal error: " << exception.what() << '\n';
        shutdown();
        return 1;
    }
}

bool Application::pressedOnce(int key) {
    if (key < 0 || key >= 512) return false;
    const bool pressed = glfwGetKey(window_, key) == GLFW_PRESS;
    const bool edge = pressed && !previousKeys_[key];
    previousKeys_[key] = pressed;
    return edge;
}

void Application::processInput(float dt) {
    ImGuiIO& io = ImGui::GetIO();
    if (pressedOnce(GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(window_, GLFW_TRUE);
    if (pressedOnce(GLFW_KEY_F1)) { showUi_ = true; ui_.toggleHelp(); }
    if (pressedOnce(GLFW_KEY_TAB)) showUi_ = !showUi_;
    const bool spacePressed = pressedOnce(GLFW_KEY_SPACE);
    if (!showUi_ && spacePressed && !io.WantTextInput) ui_.primaryAction(simulation_);

    if (!io.WantCaptureKeyboard) {
        glm::vec3 movement{0.0F};
        if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS) movement.x -= 1.0F;
        if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS) movement.x += 1.0F;
        if (glfwGetKey(window_, GLFW_KEY_Q) == GLFW_PRESS) movement.y -= 1.0F;
        if (glfwGetKey(window_, GLFW_KEY_E) == GLFW_PRESS) movement.y += 1.0F;
        if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS) movement.z -= 1.0F;
        if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS) movement.z += 1.0F;
        if (glm::length(movement) > 0.0F) {
            ui_.stopFollowing();
            camera_.move(glm::normalize(movement), dt, glfwGetKey(window_, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);
        }
        if (pressedOnce(GLFW_KEY_1)) { ui_.stopFollowing(); camera_.applyPreset(CameraPreset::Overview); }
        if (pressedOnce(GLFW_KEY_2)) { ui_.stopFollowing(); camera_.applyPreset(CameraPreset::Shearer, simulation_.shearer().position()); }
        if (pressedOnce(GLFW_KEY_3)) { ui_.stopFollowing(); camera_.applyPreset(CameraPreset::Supports); }
        if (pressedOnce(GLFW_KEY_4)) { ui_.stopFollowing(); camera_.applyPreset(CameraPreset::Conveyor, simulation_.shearer().position()); }
        if (pressedOnce(GLFW_KEY_5)) { ui_.stopFollowing(); camera_.applyPreset(CameraPreset::Entrance); }
        if (pressedOnce(GLFW_KEY_6)) { ui_.stopFollowing(); camera_.applyPreset(CameraPreset::Top); }
        if (pressedOnce(GLFW_KEY_R)) { ui_.stopFollowing(); camera_.reset(); }
        if (pressedOnce(GLFW_KEY_F12)) screenshotRequested_ = true;
        if (pressedOnce(GLFW_KEY_F)) {
            ui_.stopFollowing();
            if (selectedId_ == 1) camera_.applyPreset(CameraPreset::Shearer, simulation_.shearer().position());
            else if (selectedId_ == 2) camera_.applyPreset(CameraPreset::Conveyor);
            else if (selectedId_ >= 101 && selectedId_ < 101 + static_cast<int>(simulation_.supports().size()))
                camera_.focus({simulation_.supports()[static_cast<std::size_t>(selectedId_ - 101)].x(),
                               2.8F, layout::supportCenterZ});
        }
    }

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window_, &mouseX, &mouseY);
    const bool rightDown = glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    if (rightDown && !io.WantCaptureMouse) {
        if (mouseLookStarted_ && (mouseX != lastMouseX_ || mouseY != lastMouseY_)) {
            ui_.stopFollowing();
            camera_.rotate(static_cast<float>(mouseX - lastMouseX_), static_cast<float>(lastMouseY_ - mouseY));
        }
        mouseLookStarted_ = true;
    } else mouseLookStarted_ = false;

    const bool leftDown = glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (leftDown && !leftMouseWasDown_ && !io.WantCaptureMouse && renderer_) {
        int width = 1;
        int height = 1;
        glfwGetWindowSize(window_, &width, &height);
        selectedId_ = renderer_->pick(camera_.position(),
                                      camera_.screenRay(static_cast<float>(mouseX), static_cast<float>(mouseY), width, height));
    }
    leftMouseWasDown_ = leftDown;
    lastMouseX_ = mouseX;
    lastMouseY_ = mouseY;
}

void Application::shutdown() {
    if (uiInitialized_) {
        ui_.shutdown();
        uiInitialized_ = false;
    }
    renderer_.reset();
    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();
}

} // namespace mine
