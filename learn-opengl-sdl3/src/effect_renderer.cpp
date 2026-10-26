#include "render/effect_renderer.h"

#include "render/resource_factory.h"
#include "render/shader_contract.h"

#include <string>
#include <stdexcept>

namespace {
bool passRequires(std::span<const PassDefinition> passes, TextureSource wanted) {
    for (const PassDefinition& pass : passes)
        for (const TextureBinding& binding : pass.inputs)
            if (binding.source == wanted) return true;
    return false;
}
}

void EffectRenderer::initialize(const Effect& effect, const std::filesystem::path& shaderDirectory) {
    shutdown();
    if (const std::string validationError = validateEffect(effect, shaderDirectory); !validationError.empty()) throw std::invalid_argument(validationError);
    effect_ = &effect;
    const std::filesystem::path vertexPath = shaderDirectory / "fullscreen.vert";

    for (const PassDefinition& definition : effect.passes) {
        passes_.push_back({&definition, std::make_unique<Shader>(vertexPath.string().c_str(), (shaderDirectory / definition.fragment).string().c_str())});
    }
    if (passRequires(effect.passes, TextureSource::MatrixGlyphAtlas) || passRequires(effect.passes, TextureSource::MatrixNoise)) {
        const auto textureIds = render::createMatrixTextures();
        for (std::size_t i = 0; i < matrixTextures_.size(); ++i) matrixTextures_[i].reset(textureIds[i]);
    }
    if (passRequires(effect.passes, TextureSource::ProceduralNoise)) noiseTexture_.reset(render::createNoiseTexture());
    if (passRequires(effect.passes, TextureSource::White)) whiteTexture_.reset(render::createWhiteTexture());
    vertexArray_.create();
}

void EffectRenderer::shutdown() {
    vertexArray_.reset();
    for (auto& texture : matrixTextures_) texture.reset();
    noiseTexture_.reset();
    whiteTexture_.reset();
    bufferColorTexture_.reset();
    bufferFramebuffer_.reset();
    passes_.clear();
    effect_ = nullptr;
    bufferSize_ = {};
}

bool EffectRenderer::render(const FrameUniforms& frame, const MoonInteraction* moon) {
    if (effect_ == nullptr || passes_.empty()) return false;
    const glm::ivec2 size(frame.framebufferSize);
    if (size.x <= 0 || size.y <= 0) return true;

    glViewport(0, 0, size.x, size.y);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(vertexArray_.id());

    for (const CompiledPass& pass : passes_) {
        if (pass.definition->target == RenderTarget::Buffer) {
            if (!ensureBufferTarget(size)) return false;
            glBindFramebuffer(GL_FRAMEBUFFER, bufferFramebuffer_.id());
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        glViewport(0, 0, size.x, size.y);
        setCommonUniforms(*pass.shader, frame);
        if (moon != nullptr && pass.definition->acceptsMoonInteraction) {
            pass.shader->setFloat(std::string(shader_uniform::moonZoom), moon->zoom);
            pass.shader->setVec2(std::string(shader_uniform::moonViewCenter), moon->viewCenter);
            pass.shader->setVec3(std::string(shader_uniform::moon), moon->moon);
            for (int i = 0; i < moonRippleCount; ++i) pass.shader->setVec4("uRipples[" + std::to_string(i) + "]", moon->ripples[i]);
        }
        bindInputs(*pass.shader, *pass.definition);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return glGetError() == GL_NO_ERROR;
}

void EffectRenderer::setCommonUniforms(const Shader& shader, const FrameUniforms& frame) const {
    shader.use();
    shader.setVec2(std::string(shader_uniform::resolution), frame.framebufferSize);
    shader.setFloat(std::string(shader_uniform::time), frame.time);
    shader.setVec3(std::string(shader_uniform::shaderToyResolution), frame.framebufferSize.x, frame.framebufferSize.y, 1.0f);
    shader.setFloat(std::string(shader_uniform::shaderToyTime), frame.time);
    shader.setVec2(std::string(shader_uniform::mouse), frame.mouse);
}

void EffectRenderer::bindInputs(const Shader& shader, const PassDefinition& pass) const {
    for (const TextureBinding& binding : pass.inputs) {
        glActiveTexture(GL_TEXTURE0 + binding.unit);
        glBindTexture(GL_TEXTURE_2D, texture(binding.source));
        shader.setInt(std::string(binding.uniform), binding.unit);
        if (!binding.resolutionUniform.empty()) {
            shader.setVec3(std::string(binding.resolutionUniform), static_cast<float>(render::matrixTextureSize), static_cast<float>(render::matrixTextureSize), 1.0f);
        }
    }
}

GLuint EffectRenderer::texture(TextureSource source) const {
    switch (source) {
    case TextureSource::MatrixGlyphAtlas: return matrixTextures_[0].id();
    case TextureSource::MatrixNoise: return matrixTextures_[1].id();
    case TextureSource::ProceduralNoise: return noiseTexture_.id();
    case TextureSource::White: return whiteTexture_.id();
    case TextureSource::BufferOutput: return bufferColorTexture_.id();
    }
    return 0;
}

bool EffectRenderer::ensureBufferTarget(glm::ivec2 size) {
    if (bufferFramebuffer_.id() != 0 && bufferColorTexture_.id() != 0 && bufferSize_ == size) return true;
    bufferFramebuffer_.reset();
    bufferColorTexture_.reset();
    GLuint framebuffer = 0;
    GLuint texture = 0;
    if (!render::createBufferTarget(size.x, size.y, framebuffer, texture)) return false;
    bufferFramebuffer_.reset(framebuffer);
    bufferColorTexture_.reset(texture);
    bufferSize_ = size;
    return true;
}
