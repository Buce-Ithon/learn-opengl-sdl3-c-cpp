#include "render/effect_renderer.h"

#include "misc/example_matrix_textures.h"
#include "misc/shader_test_resources.h"

#include <string>

void EffectRenderer::initialize(const Effect& effect, const std::filesystem::path& shaderDirectory) {
    shutdown();
    effect_ = &effect;

    const std::filesystem::path vertexPath = shaderDirectory / "fullscreen.vert";
    imageShader_ = std::make_unique<Shader>(vertexPath.string().c_str(), (shaderDirectory / effect.fragment).string().c_str());

    if (effect.resources == ResourceProfile::GlyphAtlasAndNoise) {
        const auto textureIds = matrix::createTextures();
        for (std::size_t i = 0; i < matrixTextures_.size(); ++i) matrixTextures_[i].reset(textureIds[i]);
        imageShader_->use();
        imageShader_->setInt("iChannel0", 0);
        imageShader_->setInt("iChannel1", 1);
        for (int i = 0; i < 2; ++i) {
            imageShader_->setVec3("iChannelResolution[" + std::to_string(i) + "]", static_cast<float>(matrix::textureSize), static_cast<float>(matrix::textureSize), 1.0f);
        }
    } else if (effect.resources == ResourceProfile::NoiseAndWhite) {
        noiseTexture_.reset(shader_test::createNoiseTexture());
        whiteTexture_.reset(shader_test::createWhiteTexture());
    }

    if (effect.pipeline == Pipeline::TwoPass) {
        bufferShader_ = std::make_unique<Shader>(vertexPath.string().c_str(), (shaderDirectory / "up_in_cloud_sea_buffer.frag").string().c_str());
    }
    vertexArray_.create();
}

void EffectRenderer::shutdown() {
    vertexArray_.reset();
    for (auto& texture : matrixTextures_) texture.reset();
    noiseTexture_.reset();
    whiteTexture_.reset();
    bufferColorTexture_.reset();
    bufferFramebuffer_.reset();
    bufferShader_.reset();
    imageShader_.reset();
    effect_ = nullptr;
    bufferSize_ = {};
}

bool EffectRenderer::render(const FrameUniforms& frame, const MoonInteraction* moon) {
    if (effect_ == nullptr || imageShader_ == nullptr) return false;
    const glm::ivec2 size(frame.framebufferSize);
    if (size.x <= 0 || size.y <= 0) return true;

    glViewport(0, 0, size.x, size.y);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(vertexArray_.id());

    if (effect_->pipeline == Pipeline::TwoPass) {
        if (!ensureBufferTarget(size)) return false;
        glBindFramebuffer(GL_FRAMEBUFFER, bufferFramebuffer_.id());
        setCommonUniforms(*bufferShader_, frame);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, noiseTexture_.id());
        bufferShader_->setInt("iChannel0", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, whiteTexture_.id());
        bufferShader_->setInt("iChannel1", 1);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    setCommonUniforms(*imageShader_, frame);
    if (moon != nullptr) {
        imageShader_->setFloat("uZoom", moon->zoom);
        imageShader_->setVec2("uViewCenter", moon->viewCenter);
        imageShader_->setVec3("uMoon", moon->moon);
        for (int i = 0; i < moonRippleCount; ++i) imageShader_->setVec4("uRipples[" + std::to_string(i) + "]", moon->ripples[i]);
    }
    bindEffectInputs(effect_->pipeline == Pipeline::TwoPass);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    return glGetError() == GL_NO_ERROR;
}

void EffectRenderer::setCommonUniforms(const Shader& shader, const FrameUniforms& frame) const {
    shader.use();
    shader.setVec2("uResolution", frame.framebufferSize);
    shader.setFloat("uTime", frame.time);
    shader.setVec3("iResolution", frame.framebufferSize.x, frame.framebufferSize.y, 1.0f);
    shader.setFloat("iTime", frame.time);
    shader.setVec2("uMouse", frame.mouse);
}

void EffectRenderer::bindEffectInputs(bool useBufferOutput) const {
    if (effect_->resources == ResourceProfile::GlyphAtlasAndNoise) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, matrixTextures_[0].id());
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, matrixTextures_[1].id());
    } else if (effect_->resources == ResourceProfile::NoiseAndWhite) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, useBufferOutput ? bufferColorTexture_.id() : noiseTexture_.id());
        imageShader_->setInt("iChannel0", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, whiteTexture_.id());
        imageShader_->setInt("iChannel1", 1);
    }
}

bool EffectRenderer::ensureBufferTarget(glm::ivec2 size) {
    if (bufferFramebuffer_.id() != 0 && bufferColorTexture_.id() != 0 && bufferSize_ == size) return true;
    bufferFramebuffer_.reset();
    bufferColorTexture_.reset();
    GLuint framebuffer = 0;
    GLuint texture = 0;
    if (!shader_test::createBufferTarget(size.x, size.y, framebuffer, texture)) return false;
    bufferFramebuffer_.reset(framebuffer);
    bufferColorTexture_.reset(texture);
    bufferSize_ = size;
    return true;
}
