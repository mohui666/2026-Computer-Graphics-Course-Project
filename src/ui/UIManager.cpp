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
#include <cstdlib>
#include <stdexcept>

namespace mine {
namespace {
const char* stateLabel(SimulationState state) {
    switch (state) {
    case SimulationState::Stopped: return "准备就绪";
    case SimulationState::Initializing: return "正在准备设备…";
    case SimulationState::Ready: return "设备已准备好";
    case SimulationState::StartingConveyor: return "正在启动输送机…";
    case SimulationState::CuttingForward: return "正在割煤、运煤";
    case SimulationState::CuttingBackward: return "正在反向割煤";
    case SimulationState::EndTransition: return "采煤机正在换向";
    case SimulationState::Paused: return "已暂停，可以慢慢观察";
    case SimulationState::Warning: return "演示设备出现异常";
    case SimulationState::Fault: return "故障已触发，设备停止";
    case SimulationState::EmergencyStopped: return "急停已锁定";
    }
    return "";
}

const char* supportLabel(SupportStage stage) {
    switch (stage) {
    case SupportStage::Normal: return "正常支撑";
    case SupportStage::Waiting: return "等待移架";
    case SupportStage::Lowering: return "正在降架";
    case SupportStage::Advancing: return "正在移架";
    case SupportStage::Raising: return "正在升架";
    case SupportStage::LowPressure: return "压力不足";
    }
    return "";
}

bool hasFault(const SimulationController& simulation) {
    return simulation.state() == SimulationState::Fault || simulation.state() == SimulationState::Warning;
}
}

void UIManager::initialize(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;
#ifdef __APPLE__
    const std::string fontPath = "/System/Library/Fonts/Hiragino Sans GB.ttc";
#elif defined(_WIN32)
    const char* windowsDirectory = std::getenv("WINDIR");
    if (!windowsDirectory)
        throw std::runtime_error("WINDIR is unavailable; cannot locate the Chinese interface font");
    const std::string fontPath = std::string(windowsDirectory) + "/Fonts/msyh.ttc";
#else
    const std::string fontPath = "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc";
#endif
    int windowWidth = 1, windowHeight = 1, pixelWidth = 1, pixelHeight = 1;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glfwGetFramebufferSize(window, &pixelWidth, &pixelHeight);
    const float fontScale = static_cast<float>(pixelWidth) / std::max(windowWidth, 1);
    ImFontConfig fontConfig;
    fontConfig.OversampleH = 2;
    fontConfig.OversampleV = 1;
    if (!io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 18.0F * fontScale, &fontConfig,
                                     io.Fonts->GetGlyphRangesChineseFull()))
        throw std::runtime_error("Unable to load the Chinese interface font: " + fontPath);
    io.FontGlobalScale = 1.0F / fontScale;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 12.0F;
    style.FrameRounding = 6.0F;
    style.WindowPadding = {18, 18};
    style.FramePadding = {10, 8};
    style.ItemSpacing = {10, 10};
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.055F, 0.075F, 0.085F, 0.96F);
    style.Colors[ImGuiCol_Border] = ImVec4(0.22F, 0.29F, 0.32F, 0.75F);
    style.Colors[ImGuiCol_Text] = ImVec4(0.91F, 0.94F, 0.95F, 1.0F);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60F, 0.68F, 0.71F, 1.0F);
    style.Colors[ImGuiCol_Button] = ImVec4(0.13F, 0.19F, 0.22F, 1.0F);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.20F, 0.30F, 0.34F, 1.0F);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.16F, 0.36F, 0.41F, 1.0F);
    style.Colors[ImGuiCol_Header] = ImVec4(0.12F, 0.18F, 0.21F, 1.0F);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.11F, 0.22F, 0.25F, 1.0F);
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

void UIManager::primaryAction(SimulationController& simulation) {
    if (simulation.emergencyLatched()) return;
    switch (simulation.state()) {
    case SimulationState::Stopped:
        simulation.initialize();
        pendingStart_ = true;
        followShearer_ = true;
        break;
    case SimulationState::Ready:
        simulation.start();
        break;
    case SimulationState::Paused:
        simulation.resume();
        break;
    case SimulationState::CuttingForward:
    case SimulationState::CuttingBackward:
    case SimulationState::EndTransition:
        simulation.pause();
        break;
    case SimulationState::Warning:
    case SimulationState::Fault:
        simulation.clearAllFaults();
        restart(simulation);
        break;
    default: break;
    }
}

void UIManager::restart(SimulationController& simulation) {
    if (simulation.emergencyLatched()) return;
    simulation.reset();
    simulation.initialize();
    pendingStart_ = true;
    followShearer_ = true;
}

void UIManager::update(SimulationController& simulation, Camera& camera) {
    if (pendingStart_ && simulation.state() == SimulationState::Ready) {
        simulation.start();
        pendingStart_ = false;
    }
    if (simulation.emergencyLatched() || hasFault(simulation) || simulation.state() == SimulationState::Stopped)
        pendingStart_ = false;
    if (followShearer_) camera.applyPreset(CameraPreset::Shearer, simulation.shearer().position());
}

void UIManager::render(SimulationController& simulation, RenderConfig& config, Camera& camera,
                       int& selectedId, const RenderStats& stats, float fps, float frameMs) {
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const float width = std::min(340.0F, display.x - 24.0F);
    ImGui::SetNextWindowPos({display.x - width - 16.0F, 20.0F}, ImGuiCond_Always);
    ImGui::SetNextWindowSizeConstraints({width, 0}, {width, std::max(120.0F, display.y - 40.0F)});
    const auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar |
                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    ImGui::Begin("综采演示##main", nullptr, flags);
    ImGui::TextUnformatted("煤矿综采工作面");
    ImGui::TextDisabled("点一下开始，就能观看完整演示");
    ImGui::Spacing();
    const auto state = simulation.state();
    const bool blocked = simulation.emergencyLatched();
    const bool fault = hasFault(simulation);
    const bool preparing = state == SimulationState::Initializing || state == SimulationState::StartingConveyor;
    const ImVec4 statusColor = blocked || fault ? ImVec4(1.0F, 0.62F, 0.36F, 1.0F)
                                               : ImVec4(0.47F, 0.86F, 0.74F, 1.0F);
    ImGui::TextColored(statusColor, "%s", stateLabel(state));
    const char* action = blocked ? "请先解除急停" : preparing ? "正在准备，请稍等…" :
                         fault ? "排除故障并重新演示" : state == SimulationState::Paused ? "继续演示" :
                         state == SimulationState::Stopped || state == SimulationState::Ready ? "开始演示" : "暂停演示";
    ImGui::BeginDisabled(blocked || preparing);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.05F, 0.40F, 0.48F, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.08F, 0.52F, 0.60F, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.04F, 0.32F, 0.40F, 1));
    if (!ImGui::GetIO().WantTextInput)
        ImGui::SetNextItemShortcut(ImGuiKey_Space, ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_RouteOverFocused);
    if (ImGui::Button(action, {-1, 48})) primaryAction(simulation);
    ImGui::PopStyleColor(3);
    ImGui::EndDisabled();
    if (blocked) {
        ImGui::TextWrapped("急停会锁住设备。解除后，再点击开始演示。");
        if (ImGui::Button("解除急停", {-1, 38})) simulation.releaseEmergencyStop();
    }
    const float half = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5F;
    ImGui::BeginDisabled(blocked);
    if (ImGui::Button("重新开始", {half, 36})) restart(simulation);
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("怎么使用", {half, 36})) showHelp_ = true;

    ImGui::Spacing();
    ImGui::SeparatorText("想看什么？");
    if (ImGui::Button("看采煤机", {half, 38})) {
        followShearer_ = true;
        camera.applyPreset(CameraPreset::Shearer, simulation.shearer().position());
    }
    ImGui::SameLine();
    if (ImGui::Button("看液压支架", {half, 38})) {
        followShearer_ = false;
        camera.applyPreset(CameraPreset::Supports);
    }
    if (ImGui::Button("看运煤", {half, 38})) {
        followShearer_ = false;
        camera.applyPreset(CameraPreset::Conveyor, simulation.shearer().position());
    }
    ImGui::SameLine();
    if (ImGui::Button("全景俯视", {half, 38})) {
        followShearer_ = false;
        camera.applyPreset(CameraPreset::Top);
    }
    ImGui::Checkbox("镜头跟随采煤机", &followShearer_);
    ImGui::TextDisabled("空格：暂停 / 继续    Tab：隐藏面板");
    ImGui::Spacing();
    if (ImGui::CollapsingHeader("设备详情")) drawEquipment(simulation, camera, selectedId);
    if (ImGui::CollapsingHeader("高级设置（可以先不管）")) {
        if (ImGui::BeginTabBar("advanced")) {
            if (ImGui::BeginTabItem("画面")) {
                drawRenderSettings(config);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("故障演示")) {
                drawFaults(simulation);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("调试")) {
                float scale = simulation.timeScale();
                ImGui::SetNextItemWidth(150);
                if (ImGui::SliderFloat("演示速度", &scale, 0.1F, 4.0F, "%.1fx")) simulation.setTimeScale(scale);
                ImGui::Checkbox("线框", &config.wireframe);
                ImGui::Checkbox("坐标轴", &config.showAxes);
                ImGui::Checkbox("所选设备包围盒", &config.showBounds);
                ImGui::Checkbox("垂直同步", &config.vsync);
                ImGui::Text("帧率 %.0f | %.1f 毫秒", fps, frameMs);
                ImGui::Text("绘制次数 %d | 煤块 %zu", stats.drawCalls, simulation.coalPieces().size());
                if (ImGui::Button("停止设备", {-1, 0})) {
                    pendingStart_ = false;
                    simulation.stop();
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("日志")) {
                drawLog(simulation);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
    if (showHelp_) drawHelp();
}

void UIManager::endFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UIManager::drawEquipment(SimulationController& simulation, Camera& camera, int& selectedId) {
    if (selectedId < 0) ImGui::TextWrapped("可以用鼠标点击场景里的设备，也可以在下面选择。");
    if (ImGui::BeginListBox("##equipment", {-1, 140})) {
        if (ImGui::Selectable("采煤机", selectedId == 1)) selectedId = 1;
        if (ImGui::Selectable("刮板输送机", selectedId == 2)) selectedId = 2;
        if (ImGui::Selectable("工作面照明", selectedId == 3)) selectedId = 3;
        for (const auto& support : simulation.supports()) {
            const std::string label = "液压支架 " + std::to_string(support.id() - 100);
            if (ImGui::Selectable(label.c_str(), selectedId == support.id())) selectedId = support.id();
        }
        ImGui::EndListBox();
    }
    if (selectedId == 1) {
        const auto& shearer = simulation.shearer();
        ImGui::Text("位置 %.1f 米 | 速度 %.1f 米/秒", shearer.position(), shearer.speed());
        ImGui::Text("负载 %.0f%% | 温度 %.1f°C", shearer.load(), shearer.temperature());
    } else if (selectedId == 2) {
        ImGui::Text("链速 %.1f 米/秒", simulation.conveyor().speed());
        ImGui::Text("负载 %.0f%%", simulation.conveyor().load());
        float speed = simulation.conveyor().speedScale();
        ImGui::SetNextItemWidth(150);
        if (ImGui::SliderFloat("输送速度", &speed, 0.2F, 2.0F, "%.1fx")) simulation.conveyor().setSpeedScale(speed);
    } else if (selectedId == 3) {
        ImGui::Text("照明状态：%s", simulation.lightingFailed() ? "故障" : "正常");
    } else if (selectedId >= 101 && selectedId < 101 + static_cast<int>(simulation.supports().size())) {
        const auto& support = simulation.supports()[static_cast<std::size_t>(selectedId - 101)];
        ImGui::Text("状态：%s", supportLabel(support.stage()));
        ImGui::Text("压力 %.1f 兆帕 | 动作进度 %.0f%%", support.pressure(), support.progress() * 100);
    }
    if (ImGui::Button("把镜头对准所选设备", {-1, 0})) {
        followShearer_ = selectedId == 1;
        if (selectedId == 1) camera.applyPreset(CameraPreset::Shearer, simulation.shearer().position());
        else if (selectedId == 2) camera.applyPreset(CameraPreset::Conveyor, simulation.shearer().position());
        else if (selectedId >= 101 && selectedId < 101 + static_cast<int>(simulation.supports().size())) {
            camera.focus({simulation.supports()[static_cast<std::size_t>(selectedId - 101)].x(), 2.8F, layout::supportCenterZ});
        }
    }
}

void UIManager::drawFaults(SimulationController& simulation) {
    ImGui::TextWrapped("用于课堂演示联锁反应，正常观看不需要操作这里。");
    ImGui::BeginDisabled(simulation.emergencyLatched());
    if (ImGui::Button("模拟采煤机过热", {-1, 0})) simulation.injectFault(FaultType::ShearerOverheat);
    if (ImGui::Button("模拟输送机堵塞", {-1, 0})) simulation.injectFault(FaultType::ConveyorJam);
    ImGui::SetNextItemWidth(150);
    int supportNumber = supportFaultIndex_ + 1;
    if (ImGui::SliderInt("支架编号", &supportNumber, 1, static_cast<int>(simulation.supports().size())))
        supportFaultIndex_ = supportNumber - 1;
    if (ImGui::Button("模拟支架低压", {-1, 0})) simulation.injectFault(FaultType::SupportLowPressure, supportFaultIndex_);
    if (ImGui::Button("模拟照明故障", {-1, 0})) simulation.injectFault(FaultType::LightingFailure);
    if (ImGui::Button("清除全部故障", {-1, 0})) simulation.clearAllFaults();
    ImGui::Separator();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.52F, 0.13F, 0.10F, 1));
    if (ImGui::Button("模拟紧急停止", {-1, 38})) {
        pendingStart_ = false;
        simulation.emergencyStop();
    }
    ImGui::PopStyleColor();
    ImGui::EndDisabled();
}

void UIManager::drawRenderSettings(RenderConfig& config) {
    ImGui::SetNextItemWidth(150);
    ImGui::SliderFloat("画面亮度", &config.exposureEv, -1.0F, 1.2F, "%+.2f");
    ImGui::Checkbox("工作灯", &config.workLights);
    ImGui::Checkbox("相机照明", &config.headlamp);
    ImGui::Checkbox("工作灯阴影", &config.shadowsEnabled);
    ImGui::Checkbox("煤尘与喷雾", &config.dustEnabled);
    ImGui::Checkbox("剖开顶板，照亮内部", &config.cutaway);
    ImGui::Checkbox("远处雾气", &config.fogEnabled);
    ImGui::Checkbox("灯光辉光", &config.bloomEnabled);
    ImGui::Checkbox("平滑画面边缘", &config.fxaaEnabled);
    ImGui::Checkbox("接触阴影", &config.ssaoEnabled);
    if (ImGui::TreeNode("细节参数")) {
        ImGui::PushItemWidth(130);
        ImGui::SliderFloat("雾气浓度", &config.fogDensity, 0.0F, 0.075F, "%.3f");
        ImGui::SliderFloat("辉光强度", &config.bloomStrength, 0.0F, 0.30F, "%.2f");
        ImGui::SliderFloat("暗角强度", &config.vignetteStrength, 0.0F, 0.35F, "%.2f");
        ImGui::SliderFloat("接触阴影强度", &config.ssaoStrength, 0.0F, 0.65F, "%.2f");
        ImGui::PopItemWidth();
        ImGui::TreePop();
    }
    if (ImGui::Button("恢复默认画面", {-1, 0})) config = RenderConfig{};
}

void UIManager::drawLog(SimulationController& simulation) {
    ImGui::TextDisabled("原始事件日志，供开发调试使用");
    ImGui::Checkbox("自动滚动", &autoScroll_);
    if (ImGui::Button("清空日志")) simulation.eventLog().clear();
    ImGui::BeginChild("log", {0, 200}, ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& entry : simulation.eventLog().entries())
        ImGui::Text("[%.1f] %s: %s", entry.time, entry.source.c_str(), entry.message.c_str());
    if (autoScroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 20.0F) ImGui::SetScrollHereY(1.0F);
    ImGui::EndChild();
}

void UIManager::drawHelp() {
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos({display.x * 0.5F, display.y * 0.5F}, ImGuiCond_Appearing, {0.5F, 0.5F});
    ImGui::SetNextWindowSize({std::min(480.0F, display.x - 32.0F), 0}, ImGuiCond_Appearing);
    if (ImGui::Begin("怎么使用", &showHelp_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("只需要三步");
        ImGui::Separator();
        ImGui::TextWrapped("1. 点击「开始演示」，程序会自动准备设备并开始割煤。");
        ImGui::TextWrapped("2. 点击「看采煤机」「看液压支架」或「看运煤」切换画面。");
        ImGui::TextWrapped("3. 想看清细节就点击「暂停演示」，再点「继续演示」。");
        ImGui::Spacing();
        ImGui::TextWrapped("「重新开始」会将设备回到起点。「全景俯视」可以看到完整布局。");
        ImGui::SeparatorText("想自己走动时");
        ImGui::TextWrapped("按住鼠标右键转头，W/A/S/D 移动，Q/E 升降，Shift 加速。自己移动后，镜头会停止自动跟随。");
        ImGui::TextWrapped("迷路了就点「看采煤机」。空格暂停或继续，Tab 隐藏或显示面板，F12 截图，Esc 退出。");
        if (ImGui::Button("知道了", {-1, 40})) showHelp_ = false;
    }
    ImGui::End();
}

} // namespace mine
