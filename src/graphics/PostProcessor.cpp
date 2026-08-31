#include "graphics/PostProcessor.h"

#include <algorithm>
#include <stdexcept>

namespace mine {

PostProcessor::~PostProcessor() {
    releaseTargets();
    if (fullscreenVao_) glDeleteVertexArrays(1, &fullscreenVao_);
}

void PostProcessor::initialize(const std::string& assetDirectory) {
    const std::string vertex = assetDirectory + "/shaders/fullscreen.vert";
    bloomExtractShader_.load(vertex, assetDirectory + "/shaders/bloom_extract.frag");
    bloomBlurShader_.load(vertex, assetDirectory + "/shaders/bloom_blur.frag");
    ssaoShader_.load(vertex, assetDirectory + "/shaders/ssao.frag");
    ssaoBlurShader_.load(vertex, assetDirectory + "/shaders/ssao_blur.frag");
    toneMapShader_.load(vertex, assetDirectory + "/shaders/tonemap.frag");
    fxaaShader_.load(vertex, assetDirectory + "/shaders/fxaa.frag");
    glGenVertexArrays(1, &fullscreenVao_);

    GLint maximumSamples = 1;
    glGetIntegerv(GL_MAX_SAMPLES, &maximumSamples);
    samples_ = std::clamp(maximumSamples, 1, 4);
}

void PostProcessor::releaseTargets() {
    if (ldrColor_) glDeleteTextures(1, &ldrColor_);
    if (ldrFbo_) glDeleteFramebuffers(1, &ldrFbo_);
    glDeleteTextures(2, bloomColor_);
    glDeleteFramebuffers(2, bloomFbo_);
    glDeleteTextures(2, ssaoColor_);
    glDeleteFramebuffers(2, ssaoFbo_);
    if (hdrDepth_) glDeleteTextures(1, &hdrDepth_);
    if (hdrColor_) glDeleteTextures(1, &hdrColor_);
    if (hdrFbo_) glDeleteFramebuffers(1, &hdrFbo_);
    if (msaaDepth_) glDeleteRenderbuffers(1, &msaaDepth_);
    if (msaaColor_) glDeleteTextures(1, &msaaColor_);
    if (msaaFbo_) glDeleteFramebuffers(1, &msaaFbo_);
    ldrColor_ = ldrFbo_ = hdrColor_ = hdrDepth_ = hdrFbo_ = 0;
    msaaDepth_ = msaaColor_ = msaaFbo_ = 0;
    bloomColor_[0] = bloomColor_[1] = 0;
    bloomFbo_[0] = bloomFbo_[1] = 0;
    ssaoColor_[0] = ssaoColor_[1] = 0;
    ssaoFbo_[0] = ssaoFbo_[1] = 0;
    width_ = height_ = bloomWidth_ = bloomHeight_ = 0;
}

void PostProcessor::checkFramebuffer(const char* label) const {
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error(std::string("Incomplete OpenGL framebuffer: ") + label);
    }
}

void PostProcessor::ensureTargets(int width, int height) {
    width = std::max(width, 1);
    height = std::max(height, 1);
    if (width == width_ && height == height_) return;
    releaseTargets();
    width_ = width;
    height_ = height;
    bloomWidth_ = std::max(1, width / 2);
    bloomHeight_ = std::max(1, height / 2);

    glGenFramebuffers(1, &msaaFbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, msaaFbo_);
    glGenTextures(1, &msaaColor_);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msaaColor_);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples_, GL_RGBA16F,
                            width_, height_, GL_TRUE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D_MULTISAMPLE, msaaColor_, 0);
    glGenRenderbuffers(1, &msaaDepth_);
    glBindRenderbuffer(GL_RENDERBUFFER, msaaDepth_);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples_, GL_DEPTH24_STENCIL8,
                                     width_, height_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, msaaDepth_);
    checkFramebuffer("multisample HDR scene");

    glGenFramebuffers(1, &hdrFbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFbo_);
    glGenTextures(1, &hdrColor_);
    glBindTexture(GL_TEXTURE_2D, hdrColor_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width_, height_, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrColor_, 0);
    glGenTextures(1, &hdrDepth_);
    glBindTexture(GL_TEXTURE_2D, hdrDepth_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width_, height_, 0,
                 GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
                           hdrDepth_, 0);
    checkFramebuffer("resolved HDR scene");

    glGenFramebuffers(2, bloomFbo_);
    glGenTextures(2, bloomColor_);
    for (int index = 0; index < 2; ++index) {
        glBindFramebuffer(GL_FRAMEBUFFER, bloomFbo_[index]);
        glBindTexture(GL_TEXTURE_2D, bloomColor_[index]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, bloomWidth_, bloomHeight_, 0,
                     GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               bloomColor_[index], 0);
        checkFramebuffer(index == 0 ? "bloom target A" : "bloom target B");
    }

    glGenFramebuffers(2, ssaoFbo_);
    glGenTextures(2, ssaoColor_);
    for (int index = 0; index < 2; ++index) {
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFbo_[index]);
        glBindTexture(GL_TEXTURE_2D, ssaoColor_[index]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, bloomWidth_, bloomHeight_, 0,
                     GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               ssaoColor_[index], 0);
        checkFramebuffer(index == 0 ? "SSAO target A" : "SSAO target B");
    }

    glGenFramebuffers(1, &ldrFbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, ldrFbo_);
    glGenTextures(1, &ldrColor_);
    glBindTexture(GL_TEXTURE_2D, ldrColor_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ldrColor_, 0);
    checkFramebuffer("tone mapped scene");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcessor::beginScene(int width, int height, const glm::vec3& clearColor) {
    ensureTargets(width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, msaaFbo_);
    glViewport(0, 0, width_, height_);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void PostProcessor::drawFullscreenTriangle() const {
    glBindVertexArray(fullscreenVao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void PostProcessor::endScene(const RenderConfig& config, const glm::mat4& projection) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFbo_);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, hdrFbo_);
    glBlitFramebuffer(0, 0, width_, height_, 0, 0, width_, height_,
                      GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDisable(GL_MULTISAMPLE);

    glViewport(0, 0, bloomWidth_, bloomHeight_);

    int ssaoSourceIndex = 0;
    if (config.ssaoEnabled && config.ssaoStrength > 0.001F) {
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFbo_[0]);
        ssaoShader_.use();
        ssaoShader_.set("uDepth", 0);
        ssaoShader_.set("uProjection", projection);
        ssaoShader_.set("uInverseProjection", glm::inverse(projection));
        ssaoShader_.set("uRadius", 0.72F);
        ssaoShader_.set("uBias", 0.035F);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrDepth_);
        drawFullscreenTriangle();

        ssaoBlurShader_.use();
        ssaoBlurShader_.set("uOcclusion", 0);
        ssaoBlurShader_.set("uDepth", 1);
        for (int pass = 0; pass < 2; ++pass) {
            const int targetIndex = 1 - ssaoSourceIndex;
            glBindFramebuffer(GL_FRAMEBUFFER, ssaoFbo_[targetIndex]);
            ssaoBlurShader_.set("uHorizontal", pass == 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ssaoColor_[ssaoSourceIndex]);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, hdrDepth_);
            drawFullscreenTriangle();
            ssaoSourceIndex = targetIndex;
        }
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFbo_[0]);
        glClearColor(1.0F,1.0F,1.0F,1.0F);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, bloomFbo_[0]);
    bloomExtractShader_.use();
    bloomExtractShader_.set("uScene", 0);
    bloomExtractShader_.set("uThreshold", 0.90F);
    bloomExtractShader_.set("uSoftKnee", 0.45F);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrColor_);
    drawFullscreenTriangle();

    constexpr int blurPasses = 6;
    int sourceIndex = 0;
    bloomBlurShader_.use();
    bloomBlurShader_.set("uImage", 0);
    for (int pass = 0; pass < blurPasses; ++pass) {
        const int targetIndex = 1 - sourceIndex;
        glBindFramebuffer(GL_FRAMEBUFFER, bloomFbo_[targetIndex]);
        bloomBlurShader_.set("uHorizontal", (pass % 2) == 0);
        glBindTexture(GL_TEXTURE_2D, bloomColor_[sourceIndex]);
        drawFullscreenTriangle();
        sourceIndex = targetIndex;
    }

    glViewport(0, 0, width_, height_);
    glBindFramebuffer(GL_FRAMEBUFFER, ldrFbo_);
    toneMapShader_.use();
    toneMapShader_.set("uScene", 0);
    toneMapShader_.set("uBloom", 1);
    toneMapShader_.set("uOcclusion", 2);
    toneMapShader_.set("uExposureEv", config.exposureEv);
    toneMapShader_.set("uBloomStrength", config.bloomEnabled ? config.bloomStrength : 0.0F);
    toneMapShader_.set("uVignetteStrength", config.vignetteStrength);
    toneMapShader_.set("uSsaoStrength", config.ssaoEnabled ? config.ssaoStrength : 0.0F);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrColor_);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bloomColor_[sourceIndex]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, ssaoColor_[ssaoSourceIndex]);
    drawFullscreenTriangle();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width_, height_);
    fxaaShader_.use();
    fxaaShader_.set("uImage", 0);
    fxaaShader_.set("uEnabled", config.fxaaEnabled);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ldrColor_);
    drawFullscreenTriangle();

    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glEnable(GL_MULTISAMPLE);
}

} // namespace mine
