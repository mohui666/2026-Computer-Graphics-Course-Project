#pragma once

#include <algorithm>

namespace mine {

namespace layout {
inline constexpr float conveyorCenterZ = -4.35F;
inline constexpr float supportCenterZ = -1.35F;
inline constexpr float coalFaceSurfaceZ = -5.94F;
inline constexpr float conveyorHalfWidth = 1.23F;
inline constexpr float supportBaseHalfDepth = 1.475F;
inline constexpr float supportAdvanceDistance = 0.18F;
inline constexpr float scraperSpacing = 3.2F;
inline constexpr float conveyorGuideTopY = 0.84F;
inline constexpr float shearerDrumRadius = 1.26F;
inline constexpr float shearerCutterEnvelopeRadius = 1.48F;
inline constexpr float roofUndersideY = 6.02F;
}

struct SimulationConfig {
    int supportCount = 30;
    float faceLength = 72.0F;
    float shearerSpeed = 5.0F;
    float conveyorSpeed = 1.0F;
    float supportTriggerDelay = 0.8F;
    float supportStageDuration = 0.65F;
    int maxCoalPieces = 220;

    void sanitize() {
        supportCount = std::clamp(supportCount, 12, 64);
        faceLength = std::clamp(faceLength, 36.0F, 160.0F);
        shearerSpeed = std::clamp(shearerSpeed, 0.5F, 12.0F);
        conveyorSpeed = std::clamp(conveyorSpeed, 0.1F, 3.0F);
        supportTriggerDelay = std::clamp(supportTriggerDelay, 0.1F, 5.0F);
        supportStageDuration = std::clamp(supportStageDuration, 0.15F, 3.0F);
        maxCoalPieces = std::clamp(maxCoalPieces, 32, 1000);
    }
};

struct RenderConfig {
    bool fogEnabled = true;
    float fogDensity = 0.026F;
    bool headlamp = true;
    bool workLights = true;
    bool bloomEnabled = true;
    bool fxaaEnabled = true;
    bool ssaoEnabled = true;
    float exposureEv = 0.15F;
    float bloomStrength = 0.10F;
    float vignetteStrength = 0.15F;
    float ssaoStrength = 0.34F;
    bool wireframe = false;
    bool showAxes = false;
    bool showBounds = false;
    bool vsync = true;
};

} // namespace mine
