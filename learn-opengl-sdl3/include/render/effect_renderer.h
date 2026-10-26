#pragma once

#include "core/gl_objects.h"
#include "core/shader.h"
#include "effects/effect.h"
#include "effects/moon_controller.h"

#include <array>
#include <filesystem>
#include <memory>

struct FrameUniforms {
    glm::vec2 framebufferSize{};
    glm::vec2 mouse{};
    float time = 0.0f;
};

class EffectRenderer {
public:
    void initialize(const Effect& effect, const std::filesystem::path& shaderDirectory);
    void shutdown();

    [[nodiscard]] bool render(const FrameUniforms& frame, const MoonInteraction* moon);

private:
    const Effect* effect_ = nullptr;
    std::unique_ptr<Shader> imageShader_;
    std::unique_ptr<Shader> bufferShader_;
    std::array<Texture2D, 2> matrixTextures_{};
    Texture2D noiseTexture_;
    Texture2D whiteTexture_;
    Framebuffer bufferFramebuffer_;
    Texture2D bufferColorTexture_;
    VertexArray vertexArray_;
    glm::ivec2 bufferSize_{};

    void setCommonUniforms(const Shader& shader, const FrameUniforms& frame) const;
    void bindEffectInputs(bool useBufferOutput) const;
    [[nodiscard]] bool ensureBufferTarget(glm::ivec2 size);
};
