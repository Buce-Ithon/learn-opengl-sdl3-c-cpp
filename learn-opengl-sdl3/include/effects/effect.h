#pragma once

#include <span>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

enum class InteractionMode { None, Moon };
enum class RenderTarget { Window, Buffer };
enum class TextureSource { MatrixGlyphAtlas, MatrixNoise, ProceduralNoise, White, BufferOutput };

struct TextureBinding {
    std::string_view uniform;
    TextureSource source;
    int unit;
    std::string_view resolutionUniform{};
};

struct PassDefinition {
    std::string_view fragment;
    RenderTarget target = RenderTarget::Window;
    std::span<const TextureBinding> inputs{};
    bool acceptsMoonInteraction = false;
};

struct Effect {
    std::string_view name;
    std::string_view description;
    std::span<const PassDefinition> passes;
    InteractionMode interaction = InteractionMode::None;
};

[[nodiscard]] std::span<const Effect> availableEffects();
[[nodiscard]] const Effect* findEffect(std::string_view name);
[[nodiscard]] std::string validateEffect(const Effect& effect, const std::filesystem::path& shaderDirectory);
[[nodiscard]] std::vector<std::string> validateEffectRegistry(const std::filesystem::path& shaderDirectory);
