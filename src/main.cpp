#include "core/Application.h"

#include <string>

int main(int argc, char** argv) {
    bool captureMode = false;
    bool captureUi = false;
    float captureTime = 4.0F;
    std::string captureOutput = "screenshots/latest.bmp";
    mine::CameraPreset capturePreset = mine::CameraPreset::Shearer;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument.rfind("--capture-time=", 0) == 0) captureTime = std::stof(argument.substr(15));
        else if (argument.rfind("--capture-output=", 0) == 0) captureOutput = argument.substr(17);
        else if (argument == "--capture") captureMode = true;
        else if (argument == "--capture-ui") { captureMode = true; captureUi = true; }
        else if (argument == "--capture-view=overview") capturePreset = mine::CameraPreset::Overview;
        else if (argument == "--capture-view=shearer") capturePreset = mine::CameraPreset::Shearer;
        else if (argument == "--capture-view=supports") capturePreset = mine::CameraPreset::Supports;
        else if (argument == "--capture-view=conveyor") capturePreset = mine::CameraPreset::Conveyor;
        else if (argument == "--capture-view=entrance") capturePreset = mine::CameraPreset::Entrance;
        else if (argument == "--capture-view=top") capturePreset = mine::CameraPreset::Top;
    }
    mine::Application application(captureMode, capturePreset, captureTime, captureOutput, captureUi);
    return application.run();
}
