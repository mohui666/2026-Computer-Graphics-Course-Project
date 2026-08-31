#include "core/Application.h"

#include <string>

int main(int argc, char** argv) {
    bool captureMode = false;
    mine::CameraPreset capturePreset = mine::CameraPreset::Shearer;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--capture") captureMode = true;
        else if (argument == "--capture-view=overview") capturePreset = mine::CameraPreset::Overview;
        else if (argument == "--capture-view=shearer") capturePreset = mine::CameraPreset::Shearer;
        else if (argument == "--capture-view=supports") capturePreset = mine::CameraPreset::Supports;
        else if (argument == "--capture-view=conveyor") capturePreset = mine::CameraPreset::Conveyor;
        else if (argument == "--capture-view=entrance") capturePreset = mine::CameraPreset::Entrance;
        else if (argument == "--capture-view=top") capturePreset = mine::CameraPreset::Top;
    }
    mine::Application application(captureMode, capturePreset);
    return application.run();
}
