#pragma once

#include "core/Config.h"
#include "graphics/Shader.h"

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>

namespace mine {

class PostProcessor {
public:
    PostProcessor() = default;
    ~PostProcessor();
    PostProcessor(const PostProcessor&) = delete;
    PostProcessor& operator=(const PostProcessor&) = delete;

    void initialize(const std::string& assetDirectory);
    void beginScene(int width, int height, const glm::vec3& clearColor);
    void endScene(const RenderConfig& config, const glm::mat4& projection);

private:
    void ensureTargets(int width, int height);
    void releaseTargets();
    void drawFullscreenTriangle() const;
    void checkFramebuffer(const char* label) const;

    Shader bloomExtractShader_;
    Shader bloomBlurShader_;
    Shader ssaoShader_;
    Shader ssaoBlurShader_;
    Shader toneMapShader_;
    Shader fxaaShader_;
    GLuint fullscreenVao_ = 0;

    GLuint msaaFbo_ = 0;
    GLuint msaaColor_ = 0;
    GLuint msaaDepth_ = 0;
    GLuint hdrFbo_ = 0;
    GLuint hdrColor_ = 0;
    GLuint hdrDepth_ = 0;
    GLuint bloomFbo_[2]{};
    GLuint bloomColor_[2]{};
    GLuint ssaoFbo_[2]{};
    GLuint ssaoColor_[2]{};
    GLuint ldrFbo_ = 0;
    GLuint ldrColor_ = 0;

    int width_ = 0;
    int height_ = 0;
    int bloomWidth_ = 0;
    int bloomHeight_ = 0;
    int samples_ = 1;
};

} // namespace mine
