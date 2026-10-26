#pragma once

#include "core/gl_objects.h"
#include "core/shader.h"
#include "effects/effect.h"
#include "effects/moon_controller.h"
#include "render/parameters.h"

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

struct FrameUniforms {
    glm::vec2 framebufferSize{};
    glm::vec2 mouse{};
    float time = 0.0f;
};

class EffectRenderer {
public:
    void initialize(const Effect& effect, const std::filesystem::path& shaderDirectory);
    void shutdown();

    // Builds a new effect without touching the current one; on failure the previous
    // effect keeps rendering and `error` explains what went wrong.
    [[nodiscard]] bool load(const Effect& effect, const std::filesystem::path& shaderDirectory, std::string& error);
    // Recompiles the current effect from disk, preserving matching parameter values.
    [[nodiscard]] bool reload(const std::filesystem::path& shaderDirectory, std::string& error);

    [[nodiscard]] bool render(const FrameUniforms& frame, const MoonInteraction* moon);

    [[nodiscard]] const Effect* effect() const noexcept { return effect_; }
    [[nodiscard]] const ParameterSet& parameters() const noexcept { return parameters_; }
    [[nodiscard]] ParameterSet& parameters() noexcept { return parameters_; }

private:
    const Effect* effect_ = nullptr;
    struct CompiledPass {
        const PassDefinition* definition;
        std::unique_ptr<Shader> shader;
    };

    std::vector<CompiledPass> passes_;
    std::array<Texture2D, 2> matrixTextures_{};
    Texture2D noiseTexture_;
    Texture2D whiteTexture_;
    Framebuffer bufferFramebuffer_;
    Texture2D bufferColorTexture_;
    VertexArray vertexArray_;
    glm::ivec2 bufferSize_{};
    ParameterSet parameters_;

    [[nodiscard]] bool loadImpl(const Effect& effect, const std::filesystem::path& shaderDirectory, bool inheritParameters, std::string& error);
    void setCommonUniforms(const Shader& shader, const FrameUniforms& frame) const;
    void bindInputs(const Shader& shader, const PassDefinition& pass) const;
    [[nodiscard]] GLuint texture(TextureSource source) const;
    [[nodiscard]] bool ensureBufferTarget(glm::ivec2 size);
};
