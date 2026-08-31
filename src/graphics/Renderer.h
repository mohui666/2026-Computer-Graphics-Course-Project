#pragma once

#include "core/Config.h"
#include "graphics/Camera.h"
#include "graphics/Mesh.h"
#include "graphics/PostProcessor.h"
#include "graphics/Shader.h"

#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace mine {
class ParticleSystem;
class SimulationController;

struct Material {
    glm::vec3 color{0.5F};
    float metallic = 0.0F;
    float roughness = 0.7F;
    float emission = 0.0F;
    float alpha = 1.0F;
    int pattern = 0;
};

struct RenderStats {
    int drawCalls = 0;
    int visibleObjects = 0;
};

struct Pickable {
    int id = -1;
    glm::vec3 minimum{0.0F};
    glm::vec3 maximum{0.0F};
};

class Renderer {
public:
    Renderer() = default;
    void initialize(const std::string& assetDirectory);
    void render(const SimulationController& simulation, const Camera& camera,
                const ParticleSystem& particles, const RenderConfig& config,
                int width, int height, int selectedId);
    bool saveScreenshotBmp(const std::string& path, int width, int height) const;
    [[nodiscard]] int pick(const glm::vec3& origin, const glm::vec3& direction) const;
    [[nodiscard]] const RenderStats& stats() const { return stats_; }

private:
    void drawBox(const glm::vec3& position, const glm::vec3& scale, const Material& material,
                 int id = -1, const glm::vec3& eulerRadians = {0.0F, 0.0F, 0.0F});
    void drawChamferedBox(const glm::vec3& position, const glm::vec3& scale, const Material& material,
                          int id = -1, const glm::vec3& eulerRadians = {0.0F, 0.0F, 0.0F});
    void drawCylinder(const glm::vec3& position, const glm::vec3& scale, const Material& material,
                      int id = -1, const glm::vec3& eulerRadians = {0.0F, 0.0F, 0.0F});
    void drawBoxBetween(const glm::vec3& start, const glm::vec3& end, float width, float depth,
                        const Material& material, int id = -1);
    void drawCylinderBetween(const glm::vec3& start, const glm::vec3& end, float radius,
                             const Material& material, int id = -1);
    void drawMesh(const Mesh& mesh, const glm::mat4& model, const Material& material, int id);
    void drawEnvironment(const SimulationController& simulation);
    void drawSupports(const SimulationController& simulation);
    void drawShearer(const SimulationController& simulation);
    void drawConveyor(const SimulationController& simulation);
    void drawParticles(const SimulationController& simulation, const ParticleSystem& particles);
    void drawDebug(const SimulationController& simulation, const RenderConfig& config);

    Shader shader_;
    PostProcessor postProcessor_;
    Mesh cube_;
    Mesh chamferedCube_;
    Mesh cylinder_;
    Mesh rock_;
    Mesh floorSurface_;
    Mesh roofSurface_;
    Mesh coalSurface_;
    Mesh wallSurface_;
    Mesh supportShield_;
    Mesh helicalVanes_;
    RenderStats stats_;
    std::vector<Pickable> pickables_;
    int selectedId_ = -1;
    glm::vec3 cameraPosition_{0.0F};
};

} // namespace mine
