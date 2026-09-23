#include "core/Platform.h"

#include <stdexcept>
#include <string>
#include <cstdlib>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace mine {
std::filesystem::path assetDirectory() {
#ifdef __APPLE__
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string executable(size, '\0');
    if (_NSGetExecutablePath(executable.data(), &size) != 0)
        throw std::runtime_error("Cannot locate the application bundle");
    const auto directory = std::filesystem::canonical(executable.c_str()).parent_path();
    return directory.parent_path() / "Resources" / "assets";
#elif defined(_WIN32)
    std::wstring executable(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()));
    if (length == 0 || length >= executable.size())
        throw std::runtime_error("Cannot locate the executable");
    executable.resize(length);
    return std::filesystem::path(executable).parent_path() / "assets";
#else
    return std::filesystem::canonical("/proc/self/exe").parent_path() / "assets";
#endif
}

std::filesystem::path screenshotPath() {
#ifdef __APPLE__
    const char* homeDirectory = std::getenv("HOME");
    if (!homeDirectory) throw std::runtime_error("HOME is unavailable; cannot locate Pictures");
    return std::filesystem::path(homeDirectory) / "Pictures" / "MineLongwallSimulation" / "latest.bmp";
#else
    return assetDirectory().parent_path() / "screenshots" / "latest.bmp";
#endif
}
}
