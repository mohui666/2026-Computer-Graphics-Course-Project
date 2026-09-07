#include "graphics/Renderer.h"

#include "particles/ParticleSystem.h"
#include "simulation/SimulationController.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace mine {
namespace {
const Material coal{{0.025F, 0.029F, 0.032F}, 0.24F, 0.66F, 0.0F, 1.0F, 1};
const Material rock{{0.086F, 0.071F, 0.059F}, 0.01F, 0.96F, 0.0F, 1.0F, 2};
const Material darkMetal{{0.064F, 0.073F, 0.076F}, 0.88F, 0.31F, 0.0F, 1.0F, 3};
const Material steel{{0.22F, 0.26F, 0.28F}, 0.88F, 0.28F, 0.0F, 1.0F, 3};
const Material paintedBlue{{0.20F, 0.23F, 0.21F}, 0.12F, 0.67F, 0.0F, 1.0F, 4};
const Material paintedRed{{0.31F, 0.052F, 0.029F}, 0.08F, 0.52F, 0.0F, 1.0F, 4};
const Material warning{{0.36F, 0.115F, 0.004F}, 0.06F, 0.64F, 0.0F, 1.0F, 6};
const Material rubber{{0.012F, 0.015F, 0.016F}, 0.0F, 0.98F, 0.0F, 1.0F, 5};
const Material lamp{{0.84F, 0.91F, 1.0F}, 0.05F, 0.22F, 1.8F, 1.0F, 7};
const Material rust{{0.21F, 0.075F, 0.025F}, 0.48F, 0.62F, 0.0F, 1.0F, 3};
const Material wetRock{{0.043F, 0.039F, 0.035F}, 0.05F, 0.16F, 0.0F, 0.82F, 2};
const Material hose{{0.008F, 0.012F, 0.013F}, 0.08F, 0.82F, 0.0F, 1.0F, 5};
const Material contactShadow{{0.003F, 0.004F, 0.004F}, 0.0F, 1.0F, 0.0F, 0.30F, 5};
const Material drumSurface{{0.028F, 0.034F, 0.036F}, 0.91F, 0.27F, 0.0F, 1.0F, 8};
const Material drumRim{{0.078F, 0.038F, 0.021F}, 0.72F, 0.50F, 0.0F, 1.0F, 3};

glm::mat4 modelMatrix(const glm::vec3& position, const glm::vec3& scale, const glm::vec3& euler) {
    glm::mat4 model = glm::translate(glm::mat4(1.0F), position);
    model = glm::rotate(model, euler.y, {0, 1, 0});
    model = glm::rotate(model, euler.x, {1, 0, 0});
    model = glm::rotate(model, euler.z, {0, 0, 1});
    return glm::scale(model, scale);
}

bool rayAabb(const glm::vec3& origin, const glm::vec3& direction, const Pickable& box, float& distance) {
    float tMin = 0.0F;
    float tMax = std::numeric_limits<float>::max();
    for (int axis = 0; axis < 3; ++axis) {
        if (std::abs(direction[axis]) < 0.00001F) {
            if (origin[axis] < box.minimum[axis] || origin[axis] > box.maximum[axis]) return false;
        } else {
            float t1 = (box.minimum[axis] - origin[axis]) / direction[axis];
            float t2 = (box.maximum[axis] - origin[axis]) / direction[axis];
            if (t1 > t2) std::swap(t1, t2);
            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);
            if (tMin > tMax) return false;
        }
    }
    distance = tMin;
    return true;
}
}

Renderer::~Renderer() {
    if (shadowTexture_) glDeleteTextures(1, &shadowTexture_);
    if (shadowFbo_) glDeleteFramebuffers(1, &shadowFbo_);
}

void Renderer::initialize(const std::string& assetDirectory) {
    shader_.load(assetDirectory + "/shaders/mine.vert", assetDirectory + "/shaders/mine.frag");
    postProcessor_.initialize(assetDirectory);
    shadowShader_.load(assetDirectory + "/shaders/shadow.vert", assetDirectory + "/shaders/shadow.frag");
    glGenFramebuffers(1, &shadowFbo_);
    glGenTextures(1, &shadowTexture_);
    glBindTexture(GL_TEXTURE_2D_ARRAY, shadowTexture_);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT24, 2048, 2048, 2, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float border[] = {1, 1, 1, 1};
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, border);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo_);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowTexture_, 0, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Incomplete work-light shadow framebuffer");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    cube_ = Mesh::cube();
    chamferedCube_ = Mesh::chamferedBox();
    cylinder_ = Mesh::cylinder(20);
    chainLink_ = Mesh::chainLink();
    particleQuad_ = Mesh::billboard();
    rock_ = Mesh::rock();
    floorSurface_ = Mesh::roughPlane(72, 20, 17);
    roofSurface_ = Mesh::roughPlane(64, 18, 31);
    coalSurface_ = Mesh::roughPlane(280, 56, 53);
    wallSurface_ = Mesh::roughPlane(64, 16, 79);
    supportShield_ = Mesh::supportShield();
    helicalVanes_ = Mesh::helicalDrumVanes();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::render(const SimulationController& simulation, const Camera& camera,
                      const ParticleSystem& particles, const RenderConfig& config,
                      int width, int height, int selectedId) {
    stats_ = {};
    pickables_.clear();
    selectedId_ = selectedId;
    cameraPosition_ = camera.position();
    cutaway_ = config.cutaway || cameraPosition_.y > 10.0F;
    commands_.clear();
    collecting_ = true;
    drawEnvironment(simulation);
    drawSupports(simulation);
    drawConveyor(simulation);
    drawShearer(simulation);
    collecting_ = false;
    if (config.shadowsEnabled && config.workLights && !simulation.lightingFailed()) renderShadows(config);
    postProcessor_.beginScene(width, height, {0.009F, 0.012F, 0.015F});
    glPolygonMode(GL_FRONT_AND_BACK, config.wireframe ? GL_LINE : GL_FILL);

    shader_.use();
    shader_.set("uView", camera.viewMatrix());
    shader_.set("uProjection", camera.projectionMatrix(static_cast<float>(width) / std::max(1, height)));
    shader_.set("uCameraPosition", camera.position());
    shader_.set("uCameraForward", camera.forward());
    shader_.set("uHeadlamp", config.headlamp);
    shader_.set("uWorkLights", config.workLights);
    shader_.set("uLightingFailed", simulation.lightingFailed());
    shader_.set("uFogEnabled", config.fogEnabled);
    const float viewFogScale = camera.position().y > 12.0F ? 0.16F : 1.0F;
    shader_.set("uFogDensity", config.fogDensity * viewFogScale);
    shader_.set("uFogColor", glm::vec3(0.035F, 0.042F, 0.045F));
    shader_.set("uTime", static_cast<float>(simulation.simulationTime()));
    shader_.set("uShearerX", simulation.shearer().position());
    shader_.set("uCutaway", cutaway_);
    shader_.set("uWorkLightY", layout::workLightY);
    shader_.set("uWorkLightZ", layout::workLightZ);
    shader_.set("uShadowsEnabled", config.shadowsEnabled);
    shader_.set("uShadowMaps", 5);
    shader_.set("uShadowMatrix[0]", shadowMatrices_[0]);
    shader_.set("uShadowMatrix[1]", shadowMatrices_[1]);
    shader_.set("uShadowIndex0", shadowLightIndices_[0]);
    shader_.set("uShadowIndex1", shadowLightIndices_[1]);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D_ARRAY, shadowTexture_);

    for (const auto& command : commands_)
        drawMesh(*command.mesh, command.model, command.material, command.id);
    shader_.set("uDustEnabled", config.dustEnabled);
    drawParticles(simulation, particles);
    drawDebug(simulation, config);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    postProcessor_.endScene(config,
                            camera.projectionMatrix(static_cast<float>(width) / std::max(1, height)));
}

void Renderer::drawMesh(const Mesh& mesh, const glm::mat4& model, const Material& material, int id) {
    if (collecting_) {
        commands_.push_back({&mesh, model, material, id});
        if (id >= 0) {
            const glm::vec3 center(model[3]);
            const glm::vec3 extent = 0.5F * (glm::abs(glm::vec3(model[0])) +
                glm::abs(glm::vec3(model[1])) + glm::abs(glm::vec3(model[2])));
            pickables_.push_back({id, center - extent, center + extent});
        }
        return;
    }
    shader_.set("uModel", model);
    shader_.set("uNormalMatrix", glm::mat3(glm::transpose(glm::inverse(model))));
    shader_.set("uBaseColor", material.color);
    shader_.set("uMetallic", material.metallic);
    shader_.set("uRoughness", material.roughness);
    shader_.set("uEmission", material.emission);
    shader_.set("uAlpha", material.alpha);
    shader_.set("uMaterialKind", material.pattern);
    shader_.set("uSelected", id >= 0 && id == selectedId_);
    const glm::vec3 objectScale{glm::length(glm::vec3(model[0])),
                                glm::length(glm::vec3(model[1])),
                                glm::length(glm::vec3(model[2]))};
    const glm::vec3 objectPosition = glm::vec3(model[3]);
    const float seedSource = id >= 0
        ? static_cast<float>(id) * 0.6180339F
        : glm::dot(objectPosition, glm::vec3(12.9898F, 78.233F, 37.719F));
    const float objectSeed = std::sin(seedSource) * 43758.5453F;
    shader_.set("uObjectScale", objectScale);
    shader_.set("uObjectSeed", objectSeed - std::floor(objectSeed));
    mesh.draw();
    ++stats_.drawCalls;
    ++stats_.visibleObjects;
}

void Renderer::renderShadows(const RenderConfig&) {
    // Two nearby fixture cones use full geometry; distant fixtures retain unshadowed fill.
    const int first = std::clamp(static_cast<int>(std::floor((cameraPosition_.x + 30.0F) / 12.0F)), 0, 4);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo_);
    glViewport(0, 0, 2048, 2048);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0F, 3.0F);
    shadowShader_.use();
    for (int layer = 0; layer < 2; ++layer) {
        shadowLightIndices_[layer] = first + layer;
        const glm::vec3 light{-30.0F + 12.0F * (first + layer), layout::workLightY, layout::workLightZ};
        shadowMatrices_[layer] = glm::perspective(glm::radians(126.0F), 1.0F, 0.20F, 28.0F) *
            glm::lookAt(light, light + glm::vec3(0,-1,-0.38F), {0,0,-1});
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowTexture_, 0, layer);
        glClear(GL_DEPTH_BUFFER_BIT);
        shadowShader_.set("uLightMatrix", shadowMatrices_[layer]);
        for (const auto& command : commands_) {
            if (command.material.alpha < 1.0F || command.material.emission > 0.0F) continue;
            const float radius = 0.6F * (glm::length(glm::vec3(command.model[0])) +
                glm::length(glm::vec3(command.model[1])) + glm::length(glm::vec3(command.model[2])));
            if (glm::distance(light, glm::vec3(command.model[3])) > 24.0F + radius) continue;
            shadowShader_.set("uModel", command.model);
            command.mesh->draw();
            ++stats_.drawCalls;
        }
    }
    glDisable(GL_POLYGON_OFFSET_FILL);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::drawHose(const glm::vec3& start, const glm::vec3& control,
                        const glm::vec3& end, float diameter, const Material& material, int id) {
    glm::vec3 previous = start;
    for (int step = 1; step <= 9; ++step) {
        const float t = static_cast<float>(step) / 9.0F;
        const glm::vec3 point = (1-t)*(1-t)*start + 2*t*(1-t)*control + t*t*end;
        drawCylinderBetween(previous, point, diameter, material, id);
        previous = point;
    }
}

void Renderer::drawChamferedBox(const glm::vec3& position, const glm::vec3& scale,
                                const Material& material, int id,
                                const glm::vec3& eulerRadians) {
    drawMesh(chamferedCube_, modelMatrix(position, scale, eulerRadians), material, id);
}

void Renderer::drawBox(const glm::vec3& position, const glm::vec3& scale, const Material& material,
                       int id, const glm::vec3& eulerRadians) {
    drawMesh(cube_, modelMatrix(position, scale, eulerRadians), material, id);
}

void Renderer::drawCylinder(const glm::vec3& position, const glm::vec3& scale, const Material& material,
                            int id, const glm::vec3& eulerRadians) {
    drawMesh(cylinder_, modelMatrix(position, scale, eulerRadians), material, id);
}

void Renderer::drawBoxBetween(const glm::vec3& start, const glm::vec3& end, float width, float depth,
                              const Material& material, int id) {
    const glm::vec3 delta = end - start;
    const float length = glm::length(delta);
    if (length < 0.0001F) return;
    const glm::quat rotation = glm::rotation(glm::vec3(0, 1, 0), delta / length);
    const glm::mat4 model = glm::translate(glm::mat4(1.0F), (start + end) * 0.5F) *
                            glm::toMat4(rotation) *
                            glm::scale(glm::mat4(1.0F), glm::vec3(width, length, depth));
    drawMesh(cube_, model, material, id);
}

void Renderer::drawCylinderBetween(const glm::vec3& start, const glm::vec3& end, float radius,
                                   const Material& material, int id) {
    const glm::vec3 delta = end - start;
    const float length = glm::length(delta);
    if (length < 0.0001F) return;
    const glm::quat rotation = glm::rotation(glm::vec3(0, 1, 0), delta / length);
    const glm::mat4 model = glm::translate(glm::mat4(1.0F), (start + end) * 0.5F) *
                            glm::toMat4(rotation) *
                            glm::scale(glm::mat4(1.0F), glm::vec3(radius, length, radius));
    drawMesh(cylinder_, model, material, id);
}

void Renderer::drawEnvironment(const SimulationController& simulation) {
    const float length = simulation.config().faceLength;
    drawBox({0,-0.55F,0}, {length+16,1.0F,18}, rock);
    drawMesh(floorSurface_, modelMatrix({0,-0.04F,0}, {length+15,0.10F,17.8F}, {}), rock,-1);
    if (!cutaway_) {
        drawBox({0,6.49F,0}, {length+16,0.94F,18}, rock);
        drawMesh(roofSurface_, modelMatrix({0,6.05F,0}, {length+15,0.18F,17.8F},
                                          {glm::radians(180.0F),0,0}),rock,-1);
    }
    drawBox({0,3,-7.02F},{length+16,6.4F,1.55F},coal);
    drawMesh(coalSurface_, modelMatrix({0,3,-5.995F},{length+15,0.19F,6.0F},
                                       {glm::radians(90.0F),0,0}),coal,-1);
    // Sedimentary partings, irregular fragments and a dark floor litter band at the coal rib.
    for (int i=0;i<180;++i) {
        const float t = static_cast<float>(i);
        const float x = -length*0.5F + std::fmod(t*1.719F,length);
        const float size = 0.12F+0.33F*(0.5F+0.5F*std::sin(t*4.13F));
        drawMesh(rock_,modelMatrix({x,0.04F,-5.60F+0.32F*std::sin(t*3.2F)},
                                  {size*1.8F,size,size*1.3F},{t,t*0.4F,0}),coal,-1);
    }
    for (int band=0;band<3;++band) {
        drawMesh(wallSurface_,modelMatrix({0,0.64F+band*2.12F,-5.98F},
                  {length+15,0.038F,0.085F},{glm::radians(90.0F),0,0}),rock,-1);
    }
    // Behind the shields is caved rock (goaf), not a second occupied gallery.
    if (!cutaway_) {
        drawBox({0,2.7F,5.6F},{length,5.5F,6.2F},coal);
        for (int i=0;i<110;++i) {
            const float t=static_cast<float>(i);
            const float x=-length*0.5F+std::fmod(t*2.391F,length);
            const float y=0.55F+std::fmod(t*0.73F,4.8F);
            drawMesh(rock_,modelMatrix({x,y,2.2F+0.42F*std::sin(t)},
                      {2.1F,1.5F+0.5F*std::sin(t*3),2.0F},{t*0.2F,t,0.1F}),rock,-1);
        }
    }
    // Entry crosscuts at the two ends keep roadway services out of the shield/goaf zone.
    for (float endX : {-length*0.5F-5.7F,length*0.5F+5.7F}) {
        for (float z : {-3.0F,2.0F,7.0F}) {
            drawCylinder({endX-1.8F,2.8F,z},{0.15F,5.6F,0.15F},darkMetal);
            drawCylinder({endX+1.8F,2.8F,z},{0.15F,5.6F,0.15F},darkMetal);
            drawBox({endX,5.65F,z},{3.9F,0.18F,0.22F},steel);
        }
        drawCylinder({endX,5.20F,3.0F},{0.68F,9.0F,0.68F},rubber,-1,{glm::radians(90.0F),0,0});
        for (float z : {-1.0F,1.0F,3.0F,5.0F,7.0F})
            drawCylinder({endX,5.20F,z},{0.73F,0.07F,0.73F},steel,-1,{glm::radians(90.0F),0,0});
    }
    // Service lines run under the canopy on the protected face side of the legs.
    for (int pipe=0;pipe<3;++pipe) {
        drawCylinder({0,1.15F+pipe*0.18F,-1.98F},{0.065F,length,0.065F},
                      pipe==1?rust:hose,-1,{0,0,glm::radians(90.0F)});
    }
    for (int i=0;i<6;++i) {
        const float x=-30.0F+12.0F*i;
        drawBox({x,layout::workLightY+0.18F,layout::workLightZ},{1.20F,0.20F,0.32F},darkMetal,3);
        drawBox({x,layout::workLightY,layout::workLightZ},{0.96F,0.025F,0.22F},lamp,3);
        for(float bar : {-0.38F,0.0F,0.38F})
            drawBox({x+bar,layout::workLightY-0.026F,layout::workLightZ},{0.023F,0.026F,0.26F},steel,3);
        drawHose({x+0.6F,5.3F,-2.6F},{x+1.0F,4.9F,-1.9F},{x+1.5F,5.4F,-1.6F},0.035F,hose);
    }
    for (const glm::vec4& patch : {glm::vec4{-38,0,3.2F,1.3F},glm::vec4{-12,-2.8F,4.0F,0.4F},glm::vec4{18,-2.7F,3.8F,0.35F}})
        drawMesh(rock_,modelMatrix({patch.x,0.012F,patch.y},{patch.z,0.012F,patch.w},{}),wetRock,-1);
}

void Renderer::drawSupports(const SimulationController& simulation) {
    for (const auto& support : simulation.supports()) {
        const SupportVisualPose pose = support.visualPose();
        const float height = pose.heightScale;
        const int id = support.id();
        const glm::vec3 color = support.lowPressure() ? glm::vec3(0.62F,0.05F,0.025F) : paintedBlue.color;
        const float variation = 0.88F + 0.10F * std::sin(static_cast<float>(support.id()) * 1.71F);
        const Material frame{color * variation, 0.16F, 0.76F, 0.0F, 1.0F, 4};
        const float x = support.x();
        const float z = layout::supportCenterZ + pose.advanceOffset;
        const float canopyY = 5.72F * height;
        const float canopyWidth = simulation.config().faceLength * 0.9F / (simulation.config().supportCount - 1) - 0.045F;

        // Split sole plates, bridge base and advancing ram.
        drawChamferedBox({x - 0.50F,-0.005F,z}, {0.82F,0.008F,2.72F}, contactShadow);
        drawChamferedBox({x + 0.50F,-0.005F,z}, {0.82F,0.008F,2.72F}, contactShadow);
        drawChamferedBox({x - 0.50F, 0.17F, z}, {0.72F, 0.30F, layout::supportBaseHalfDepth * 2.0F}, frame, id);
        drawChamferedBox({x + 0.50F, 0.17F, z}, {0.72F, 0.30F, layout::supportBaseHalfDepth * 2.0F}, frame, id);
        drawChamferedBox({x, 0.34F, z - 0.45F}, {1.82F, 0.34F, 1.25F}, darkMetal, id);
        drawCylinder({x, 0.38F, z - 0.25F}, {0.20F, 1.75F, 0.20F}, steel, id,
                     {glm::radians(90.0F), 0.0F, 0.0F});
        drawCylinder({x, 0.38F, z + 0.70F}, {0.28F, 0.72F, 0.28F}, darkMetal, id,
                     {glm::radians(90.0F), 0.0F, 0.0F});

        // Inclined two-stage props with fixed barrels and extending chrome rods.
        for (float columnX : {x-0.55F,x+0.55F}) {
            const glm::vec3 foot{columnX,0.45F,z+0.20F};
            const glm::vec3 head{columnX,canopyY-0.20F,z-0.72F};
            const glm::vec3 axis=glm::normalize(head-foot);
            drawCylinderBetween(foot,foot+axis*2.85F,0.43F,frame,id);
            drawCylinderBetween(foot+axis*2.72F,head,0.29F,steel,id);
            drawCylinderBetween(foot+axis*2.72F,foot+axis*2.93F,0.51F,darkMetal,id);
            drawCylinderBetween(head-axis*0.10F,head+axis*0.10F,0.48F,steel,id);
            drawHose(foot+glm::vec3(0,2.2F,-0.25F),{columnX+0.22F,0.4F,z-1.20F},
                     {x,1.18F,z-0.8F},0.055F,hose,id);
        }

        // Canopy is layered and ribbed rather than a single cyan slab.
        drawChamferedBox({x, canopyY, z - 0.70F}, {canopyWidth, 0.34F, 4.60F}, frame, id,
                         {glm::radians(-3.0F), 0, 0});
        drawBox({x, canopyY - 0.22F, z - 0.70F}, {canopyWidth - 0.16F, 0.12F, 4.35F}, steel, id,
                {glm::radians(-3.0F), 0, 0});
        for (float ribZ : {-1.15F, 0.0F, 1.15F}) {
            drawBox({x, canopyY + 0.22F, z + ribZ}, {1.78F, 0.15F, 0.18F}, darkMetal, id,
                    {glm::radians(-3.0F), 0, 0});
        }
        drawChamferedBox({x, canopyY - 0.12F, z - 2.98F}, {canopyWidth - 0.04F, 0.25F, 0.42F}, frame, id,
                         {glm::radians(-5.0F), 0, 0});
        drawBox({x, canopyY + 0.12F, z + 1.68F}, {1.90F, 0.22F, 0.16F}, warning, id);

        drawChamferedBox({x,1.52F,z-1.18F},{0.84F,0.32F,0.28F},frame,id);
        for (int port=0;port<4;++port) {
            const float px=x-0.29F+port*0.19F;
            drawCylinder({px,1.55F,z-1.36F},{0.065F,0.10F,0.065F},steel,id,{glm::radians(90.0F),0,0});
            drawHose({px,1.49F,z-1.35F},{px+0.16F,0.55F,z-1.65F},
                      {px+0.28F,1.10F,z-0.75F},0.043F,hose,id);
        }
        for(float ribX : {-0.70F,0.70F})
            drawBox({x+ribX,canopyY-0.32F,z-0.85F},{0.12F,0.20F,3.7F},darkMetal,id);

        // A dark trapezoid shell, inset painted panel and ribs expose the mechanism instead of
        // reading as one continuous turquoise wall. Every detail follows the same animated pose.
        const float shieldAngle = glm::radians(12.6F + 0.7F * std::sin(static_cast<float>(id) * 0.61F));
        const glm::vec3 shieldCenter{x, 2.92F * height, z + 1.34F};
        const auto shieldPoint = [&](const glm::vec3& local) {
            const float cosine = std::cos(shieldAngle);
            const float sine = std::sin(shieldAngle);
            return shieldCenter + glm::vec3{local.x,
                                             local.y * cosine - local.z * sine,
                                             local.y * sine + local.z * cosine};
        };
        drawMesh(supportShield_, modelMatrix(shieldCenter, {1.78F,3.82F * height,0.30F},
                                                   {shieldAngle,0,0}), darkMetal, id);
        Material insetPanel = frame;
        insetPanel.color *= 0.92F;
        insetPanel.roughness = 0.48F;
        drawMesh(supportShield_, modelMatrix(shieldPoint({0,0.02F,0.19F}),
                                                   {1.38F,2.86F * height,0.10F},
                                                   {shieldAngle,0,0}), insetPanel, id);
        for (float ribX : {-0.69F, 0.69F}) {
            drawChamferedBox(shieldPoint({ribX,0.0F,0.27F}),
                             {0.105F,3.28F * height,0.095F}, darkMetal, id,
                             {shieldAngle,0,0});
        }
        for (float ribY : {-1.08F, 0.04F, 1.10F}) {
            drawChamferedBox(shieldPoint({0,ribY * height,0.27F}),
                             {1.42F,0.105F,0.095F}, darkMetal, id,
                             {shieldAngle,0,0});
        }
        for (float boltX : {-0.55F,0.55F}) {
            for (float boltY : {-1.13F,1.13F}) {
                drawCylinder(shieldPoint({boltX,boltY * height,0.34F}),
                             {0.085F,0.11F,0.085F}, steel, id,
                             {shieldAngle + glm::radians(90.0F),0,0});
            }
        }

        // Rear shield and lemniscate links stay on the goaf/walkway side; the coal-side aisle remains open.
        for (float linkX : {x - 0.56F, x + 0.56F}) {
            drawBox({linkX, 2.18F * height, z + 0.74F}, {0.17F, 3.25F * height, 0.20F}, steel, id,
                    {glm::radians(27.0F), 0, 0});
            drawCylinder({linkX, 0.62F, z + 0.20F}, {0.32F, 0.20F, 0.32F}, darkMetal, id,
                         {glm::radians(90.0F), 0, 0});
        }
        drawBox({x, 1.05F, z + 1.20F}, {1.18F, 0.72F, 0.34F}, darkMetal, id);
        drawBox({x, 1.06F, z + 1.40F}, {0.92F, 0.38F, 0.08F}, warning, id);
        for (int port = 0; port < 3; ++port) {
            drawCylinder({x - 0.30F + 0.30F * port, 1.08F, z + 1.48F},
                         {0.075F, 0.09F, 0.075F}, port == 1 ? paintedRed : steel, id,
                         {glm::radians(90.0F), 0, 0});
        }
    }
}

void Renderer::drawShearer(const SimulationController& simulation) {
    const auto& shearer = simulation.shearer();
    const glm::vec3 bodyPosition = shearer.transform().worldPosition();
    const glm::vec3 leftArmPosition = shearer.leftArmTransform().worldPosition();
    const glm::vec3 rightArmPosition = shearer.rightArmTransform().worldPosition();
    const glm::vec3 leftDrumPosition = shearer.leftDrumTransform().worldPosition();
    const glm::vec3 rightDrumPosition = shearer.rightDrumTransform().worldPosition();
    const float x = bodyPosition.x;
    const int id = shearer.id();
    const Material body = shearer.overheated() ? Material{{0.72F,0.03F,0.01F},0.45F,0.32F,0.35F,1}
                                               : paintedRed;
    const float z = bodyPosition.z;

    // Long, low chassis split into service bays instead of one toy-like red cube.
    drawChamferedBox({x, 1.06F, z}, {7.15F, 0.34F, 1.70F}, darkMetal, id);
    drawChamferedBox({x, 1.40F, z}, {6.75F, 0.44F, 1.56F}, body, id);
    for (int bay = -1; bay <= 1; ++bay) {
        const float bayX = x + static_cast<float>(bay) * 2.12F;
        const Material bayMaterial = bay == 0 ? darkMetal : body;
        drawChamferedBox({bayX, 1.72F + (bay == 0 ? 0.08F : 0.0F), z},
                         {1.84F, bay == 0 ? 0.76F : 0.64F, 1.36F}, bayMaterial, id,
                         {0.0F, 0.0F, glm::radians(static_cast<float>(bay) * 1.5F)});
        drawChamferedBox({bayX, 2.12F, z + 0.05F}, {1.45F, 0.13F, 1.12F}, darkMetal, id);
        if (bay == 0) {
            drawCylinder({bayX, 2.28F, z}, {0.34F, 1.18F, 0.34F}, steel, id,
                         {0.0F, 0.0F, glm::radians(90.0F)});
            drawBox({bayX, 2.23F, z + 0.58F}, {1.10F, 0.08F, 0.09F}, warning, id);
        }
    }

    // Cooling grille, access latches, cable tray and narrow safety markings.
    drawBox({x, 1.50F, z + 0.84F}, {5.85F, 0.72F, 0.12F}, rubber, id);
    for (int vent = 0; vent < 9; ++vent) {
        drawBox({x - 2.55F + vent * 0.64F, 1.50F, z + 0.925F},
                {0.12F, 0.50F, 0.055F}, steel, id, {0,0,glm::radians(-8.0F)});
    }
    drawBox({x, 1.03F, z + 0.98F}, {6.55F, 0.16F, 0.32F}, darkMetal, id);
    for (float markerX : {-2.65F, -0.88F, 0.88F, 2.65F}) {
        drawBox({x + markerX, 1.03F, z + 1.17F}, {0.42F, 0.10F, 0.08F}, warning, id,
                {0,0,glm::radians(18.0F)});
    }

    // Four low trapping shoes grip the AFC guides; no railway-like wheels.
    for (float shoeZ : {z - 1.02F, z + 1.02F}) {
        for (float shoeX : {x - 2.35F, x + 2.35F}) {
            drawBox({shoeX, 0.95F, shoeZ}, {1.45F, 0.16F, 0.42F}, darkMetal, id);
            drawBox({shoeX, 0.88F, shoeZ}, {1.15F, 0.06F, 0.54F}, steel, id);
        }
    }
    drawBox({x, 1.02F, z + 1.13F}, {5.65F, 0.14F, 0.20F}, rust, id);

    // Ranging arms really connect their pivots to the offset cutting drums.
    drawBoxBetween(leftArmPosition, leftDrumPosition, 0.58F, 0.70F, body, id);
    drawBoxBetween(rightArmPosition, rightDrumPosition, 0.58F, 0.70F, body, id);
    for (const auto& arm : {leftArmPosition, rightArmPosition}) {
        drawCylinder(arm, {0.72F, 0.42F, 0.72F}, darkMetal, id,
                     {glm::radians(90.0F), 0, 0});
        drawCylinder(arm, {0.28F, 0.45F, 0.28F}, warning, id,
                     {glm::radians(90.0F), 0, 0});
    }
    drawCylinderBetween({x - 2.35F, 1.25F, z - 0.15F},
                        glm::mix(leftArmPosition, leftDrumPosition, 0.58F), 0.14F, steel, id);
    drawCylinderBetween({x + 2.35F, 1.25F, z - 0.15F},
                        glm::mix(rightArmPosition, rightDrumPosition, 0.58F), 0.14F, steel, id);

    const float angle = shearer.drumAngle();
    for (int side : {-1,1}) {
        const glm::vec3 center=side<0?leftDrumPosition:rightDrumPosition;
        const float spin=side<0?angle:-angle;
        const auto drumPart = [&](const Mesh& mesh, const glm::vec3& offset, const glm::vec3& scale, const Material& mat) {
            glm::mat4 model=glm::translate(glm::mat4(1),center+offset);
            model=glm::rotate(model,spin,glm::vec3(0,0,1));
            model=glm::rotate(model,glm::radians(90.0F),glm::vec3(1,0,0));
            drawMesh(mesh,glm::scale(model,scale),mat,id);
        };
        drumPart(cylinder_,{}, {2.20F,1.08F,2.20F},drumSurface);
        drumPart(cylinder_,{0,0,0.06F},{0.62F,1.24F,0.62F},steel);
        drumPart(helicalVanes_,{}, {2.72F,1.04F,2.72F},drumRim);
        for(float axial : {-0.51F,0.51F})
            drumPart(cylinder_,{0,0,axial},{2.52F,0.065F,2.52F},darkMetal);
        for (int row=0;row<3;++row) {
            for(int tooth=0;tooth<14;++tooth) {
                const float a=spin+tooth*6.2831853F/14.0F+row*0.35F;
                const glm::vec3 radial{std::cos(a),std::sin(a),0};
                const glm::vec3 tangent{-std::sin(a),std::cos(a),0};
                const glm::vec3 root=center+radial*1.28F+glm::vec3(0,0,-0.50F+row*0.48F);
                drawCylinderBetween(root-tangent*0.08F,root+radial*0.17F+tangent*0.12F,0.12F,rust,id);
                drawCylinderBetween(root+radial*0.13F+tangent*0.08F,
                                    root+radial*0.19F+tangent*0.21F,0.066F,steel,id);
                if(row==0) drawCylinderBetween(root,root+glm::vec3(0,0,-0.22F),0.08F,steel,id);
            }
        }
        for(int nozzle=0;nozzle<5;++nozzle) {
            const float a=nozzle*0.45F+0.55F;
            const glm::vec3 tip=center+glm::vec3(std::cos(a)*1.13F,std::sin(a)*1.13F,0.62F);
            drawCylinderBetween(tip,tip+glm::vec3(0,0,-0.14F),0.05F,steel,id);
        }
    }
    for(int line=0;line<3;++line) {
        const float dz=line*0.095F;
        drawHose({x-3.15F,2.12F,z+0.58F+dz},{x,2.52F,z+0.88F+dz},
                  {x+3.15F,2.12F,z+0.58F+dz},0.070F,hose,id);
    }
    for(int bolt=0;bolt<12;++bolt) {
        const float bx=x-2.75F+bolt*0.50F;
        drawCylinder({bx,1.89F,z+0.73F},{0.075F,0.055F,0.075F},steel,id,{glm::radians(90.0F),0,0});
    }
    drawBox({x - 1.15F, 2.48F, z + 0.42F}, {0.46F,0.24F,0.38F}, darkMetal, id);
    drawBox({x - 1.15F, 2.40F, z + 0.62F}, {0.30F,0.12F,0.10F}, lamp, id);
    drawBox({x + 1.15F, 2.48F, z + 0.42F}, {0.46F,0.24F,0.38F}, darkMetal, id);
    drawBox({x + 1.15F, 2.40F, z + 0.62F}, {0.30F,0.12F,0.10F}, lamp, id);
}

void Renderer::drawConveyor(const SimulationController& simulation) {
    const float length = simulation.config().faceLength;
    const int id = simulation.conveyor().id();
    const float centerZ = layout::conveyorCenterZ;
    const float coalSideZ = centerZ - 1.12F;
    const float walkwaySideZ = centerZ + 1.12F;
    const float rackZ = centerZ + 1.28F;
    drawChamferedBox({0,-0.005F,centerZ}, {length,0.008F,2.54F}, contactShadow);
    drawChamferedBox({0, 0.29F, centerZ}, {length, 0.24F, layout::conveyorHalfWidth * 2.0F}, darkMetal, id);

    // Individual armored face-conveyor pans, seams and side profiles.
    for (float x = -length * 0.5F + 1.6F; x < length * 0.5F; x += 3.2F) {
        Material pan = darkMetal;
        pan.color *= 0.84F + 0.10F * std::sin(x * 0.91F);
        drawChamferedBox({x, 0.48F, centerZ}, {3.06F, 0.15F, 1.86F}, pan, id);
        drawBox({x - 1.54F, 0.55F, centerZ}, {0.08F, 0.38F, 2.30F}, rust, id);
        drawBox({x, 0.60F, coalSideZ}, {3.10F, 0.48F, 0.22F}, darkMetal, id,
                {glm::radians(-7.0F), 0, 0});
        drawBox({x, 0.60F, walkwaySideZ}, {3.10F, 0.48F, 0.22F}, darkMetal, id,
                {glm::radians(7.0F), 0, 0});
    }

    // Interlocking oval links alternate horizontal / vertical, sharing scraper motion.
    const auto scrapers = simulation.conveyor().scraperPositions(length);
    const float chainOffset = scrapers.empty() ? 0.0F : scrapers.front()+length*0.5F;
    for(float chainZ : {centerZ-0.35F,centerZ+0.35F}) {
        for(int link=0;link<static_cast<int>(length/0.20F);++link) {
            const float px=-length*0.5F+std::fmod(link*0.20F+chainOffset,length);
            drawMesh(chainLink_,modelMatrix({px,0.65F,chainZ},{0.48F,0.48F,0.48F},
                     {link%2?glm::radians(90.0F):0.0F,0,0}),steel,id);
        }
    }
    drawBox({0.0F, 0.79F, rackZ}, {length, 0.16F, 0.18F}, rust, id);
    for (float x = -length * 0.5F + 0.15F; x <= length * 0.5F - 0.15F; x += 0.55F) {
        drawBox({x, 0.89F, rackZ}, {0.24F, 0.13F, 0.24F}, steel, id,
                {0.0F, glm::radians(45.0F), 0.0F});
    }

    for(int i=0;i<130;++i) {
        const float t=static_cast<float>(i);
        const float px=-length*0.5F+std::fmod(t*0.537F,length);
        const float size=0.12F+0.13F*(0.5F+0.5F*std::sin(t*3.3F));
        drawMesh(rock_,modelMatrix({px,0.58F+size*0.32F,centerZ+0.80F*std::sin(t*4.71F)},
                                  {size*1.5F,size,size},{t,0,t*0.2F}),coal,-1);
    }
    drawChamferedBox({-length*0.5F-1.5F,0.67F,centerZ+1.95F},{2.0F,1.1F,1.2F},paintedBlue,id);
    drawCylinder({-length*0.5F-3.0F,0.72F,centerZ+1.95F},{0.88F,1.4F,0.88F},darkMetal,id,{0,0,glm::radians(90.0F)});
    for(int fin=0;fin<7;++fin)
        drawCylinder({-length*0.5F-2.4F-fin*0.18F,0.72F,centerZ+1.95F},{0.98F,0.07F,0.98F},steel,id,{0,0,glm::radians(90.0F)});
    // Drive and return sprocket housings.
    for (float endX : {-length * 0.5F, length * 0.5F}) {
        drawChamferedBox({endX, 0.58F, centerZ}, {2.15F, 0.95F, 2.70F}, paintedBlue, id);
        drawCylinder({endX, 0.72F, centerZ}, {1.35F, 2.45F, 1.35F}, darkMetal, id,
                     {glm::radians(90.0F),0,0});
        drawCylinder({endX, 0.72F, centerZ}, {0.48F, 2.52F, 0.48F}, warning, id,
                     {glm::radians(90.0F),0,0});
    }
    for (float scraperX : simulation.conveyor().scraperPositions(length)) {
        drawBox({scraperX, 0.72F, centerZ}, {0.14F, 0.17F, 1.74F}, steel, id);
        drawBox({scraperX, 0.74F, centerZ - 0.88F}, {0.22F, 0.24F, 0.20F}, warning, id);
        drawBox({scraperX, 0.74F, centerZ + 0.88F}, {0.22F, 0.24F, 0.20F}, warning, id);
        drawBox({scraperX, 0.77F, centerZ - 0.35F}, {0.24F, 0.20F, 0.24F}, darkMetal, id);
        drawBox({scraperX, 0.77F, centerZ + 0.35F}, {0.24F, 0.20F, 0.24F}, darkMetal, id);
    }
}

void Renderer::drawParticles(const SimulationController& simulation, const ParticleSystem& particles) {
    for (const auto& piece : simulation.coalPieces()) {
        Material chunk = coal;
        chunk.color *= 0.62F + 0.10F * std::sin(piece.age * 1.7F);
        drawMesh(rock_, modelMatrix(piece.position,
                                    {piece.size * 1.15F, piece.size * 0.86F, piece.size * 1.32F},
                                    {piece.age * 1.1F, piece.age * 0.7F, piece.age * 0.43F}),
                 chunk, -1);
    }
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    std::vector<const DustParticle*> sorted;
    for (const auto& particle : particles.particles()) sorted.push_back(&particle);
    std::sort(sorted.begin(),sorted.end(),[&](const DustParticle* a,const DustParticle* b){
        return glm::dot(a->position-cameraPosition_,a->position-cameraPosition_) >
               glm::dot(b->position-cameraPosition_,b->position-cameraPosition_);
    });
    for (const auto* particle : sorted) {
        const glm::vec3 view = glm::normalize(cameraPosition_-particle->position);
        const glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0,1,0),view));
        const glm::vec3 up = glm::cross(view,right);
        const float size=particle->size*(1.0F+particle->age*0.75F);
        glm::mat4 model(1.0F);
        model[0]=glm::vec4(right*size,0);
        model[1]=glm::vec4(up*size,0);
        model[2]=glm::vec4(view,0);
        model[3]=glm::vec4(particle->position,1);
        Material dust{particle->mist?glm::vec3(0.68F,0.77F,0.83F):glm::vec3(0.26F,0.25F,0.22F),
                      0,1,0,particle->alpha,particle->mist?10:9};
        drawMesh(particleQuad_,model,dust,-1);
    }
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
}

void Renderer::drawDebug(const SimulationController&, const RenderConfig& config) {
    if (config.showAxes) {
        drawBox({2.0F,0.06F,0}, {4.0F,0.08F,0.08F}, Material{{1,0.05F,0.02F},0,0.5F,1,1});
        drawBox({0,2.0F,0}, {0.08F,4.0F,0.08F}, Material{{0.05F,1,0.05F},0,0.5F,1,1});
        drawBox({0,0.06F,2.0F}, {0.08F,0.08F,4.0F}, Material{{0.05F,0.2F,1},0,0.5F,1,1});
    }
    if (config.showBounds && !config.wireframe) {
        const auto boxes = pickables_;
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_CULL_FACE);
        for (const auto& box : boxes) {
            if (box.id != selectedId_) continue;
            const glm::vec3 size = box.maximum - box.minimum;
            drawBox((box.minimum + box.maximum) * 0.5F, size * 1.03F,
                    Material{{1.0F,0.65F,0.03F},0,0.3F,1.0F,0.85F});
        }
        glEnable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

int Renderer::pick(const glm::vec3& origin, const glm::vec3& direction) const {
    int result = -1;
    float nearest = std::numeric_limits<float>::max();
    for (const auto& pickable : pickables_) {
        float distance = 0.0F;
        if (rayAabb(origin, direction, pickable, distance) && distance < nearest) {
            nearest = distance;
            result = pickable.id;
        }
    }
    return result;
}

bool Renderer::saveScreenshotBmp(const std::string& path, int width, int height) const {
    if (width <= 0 || height <= 0) return false;
    const int rowBytes = width * 3;
    const int paddedRowBytes = (rowBytes + 3) & ~3;
    std::vector<unsigned char> rgb(static_cast<std::size_t>(width * height * 3));
    std::vector<unsigned char> bgr(static_cast<std::size_t>(paddedRowBytes * height), 0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const std::size_t source = static_cast<std::size_t>((y * width + x) * 3);
            const std::size_t destination = static_cast<std::size_t>(y * paddedRowBytes + x * 3);
            bgr[destination] = rgb[source + 2];
            bgr[destination + 1] = rgb[source + 1];
            bgr[destination + 2] = rgb[source];
        }
    }

    const std::filesystem::path output = std::filesystem::u8path(path);
    if (output.has_parent_path()) std::filesystem::create_directories(output.parent_path());
    std::ofstream file(output, std::ios::binary);
    if (!file) return false;
    const std::uint32_t pixelOffset = 54;
    const std::uint32_t fileSize = pixelOffset + static_cast<std::uint32_t>(bgr.size());
    const std::uint32_t dibSize = 40;
    const std::uint16_t planes = 1;
    const std::uint16_t bitsPerPixel = 24;
    const std::uint32_t compression = 0;
    const std::uint32_t imageSize = static_cast<std::uint32_t>(bgr.size());
    const std::int32_t pixelsPerMeter = 2835;
    const std::uint32_t zero = 0;
    file.write("BM", 2);
    file.write(reinterpret_cast<const char*>(&fileSize), 4);
    file.write(reinterpret_cast<const char*>(&zero), 4);
    file.write(reinterpret_cast<const char*>(&pixelOffset), 4);
    file.write(reinterpret_cast<const char*>(&dibSize), 4);
    file.write(reinterpret_cast<const char*>(&width), 4);
    file.write(reinterpret_cast<const char*>(&height), 4);
    file.write(reinterpret_cast<const char*>(&planes), 2);
    file.write(reinterpret_cast<const char*>(&bitsPerPixel), 2);
    file.write(reinterpret_cast<const char*>(&compression), 4);
    file.write(reinterpret_cast<const char*>(&imageSize), 4);
    file.write(reinterpret_cast<const char*>(&pixelsPerMeter), 4);
    file.write(reinterpret_cast<const char*>(&pixelsPerMeter), 4);
    file.write(reinterpret_cast<const char*>(&zero), 4);
    file.write(reinterpret_cast<const char*>(&zero), 4);
    file.write(reinterpret_cast<const char*>(bgr.data()), static_cast<std::streamsize>(bgr.size()));
    return file.good();
}

} // namespace mine
