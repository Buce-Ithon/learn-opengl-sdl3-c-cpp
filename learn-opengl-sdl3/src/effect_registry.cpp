#include "effects/effect.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace {
constexpr TextureBinding matrixInputs[] = {
    {"iChannel0", TextureSource::MatrixGlyphAtlas, 0, "iChannelResolution[0]"},
    {"iChannel1", TextureSource::MatrixNoise, 1, "iChannelResolution[1]"},
};
constexpr TextureBinding noiseAndWhiteInputs[] = {
    {"iChannel0", TextureSource::ProceduralNoise, 0},
    {"iChannel1", TextureSource::White, 1},
};
constexpr TextureBinding bufferAndWhiteInputs[] = {
    {"iChannel0", TextureSource::BufferOutput, 0},
    {"iChannel1", TextureSource::White, 1},
};

constexpr PassDefinition matrixPasses[] = {{"matrix.frag", RenderTarget::Window, matrixInputs}};
constexpr PassDefinition moonPasses[] = {{"midmoon.frag", RenderTarget::Window, {}, true}};
constexpr PassDefinition cloudBufferPasses[] = {{"up_in_cloud_sea_buffer.frag", RenderTarget::Window, noiseAndWhiteInputs}};
constexpr PassDefinition cloudImagePasses[] = {
    {"up_in_cloud_sea_buffer.frag", RenderTarget::Buffer, noiseAndWhiteInputs},
    {"up_in_cloud_sea_image.frag", RenderTarget::Window, bufferAndWhiteInputs},
};
constexpr PassDefinition mtFujiPasses[] = {{"mt_fuji_watercolor.frag"}};
constexpr PassDefinition vanGoghPasses[] = {{"van_gogh_sunset.frag"}};
constexpr PassDefinition squarePasses[] = {{"square_bg_v2.frag"}};
constexpr PassDefinition paperPasses[] = {{"paper_sticks.frag"}};
constexpr PassDefinition loaderPasses[] = {{"infinity_loader.frag"}};
constexpr PassDefinition octagramPasses[] = {{"octagrams.frag"}};
constexpr PassDefinition neonPasses[] = {{"neon_infinity.frag"}};
constexpr PassDefinition cyberFujiPasses[] = {{"cyber_fuji_2020.frag"}};
constexpr PassDefinition prettyHipPasses[] = {{"pretty_hip.frag"}};

constexpr Effect effects[] = {
    {"matrix", "Character rain", matrixPasses}, {"mt_fuji_watercolor", "Mt. Fuji watercolor", mtFujiPasses},
    {"van_gogh_sunset", "Van Gogh Sunset", vanGoghPasses}, {"square_bg_v2", "Square Background V2", squarePasses},
    {"paper_sticks", "Paper Sticks", paperPasses}, {"infinity_loader", "Infinity loader", loaderPasses},
    {"octagrams", "Octagrams", octagramPasses}, {"neon_infinity", "NEON INFINITY", neonPasses},
    {"cyber_fuji_2020", "Cyber Fuji 2020", cyberFujiPasses}, {"pretty_hip", "Pretty Hip", prettyHipPasses},
    {"midmoon", "Interactive Moonrise", moonPasses, InteractionMode::Moon},
    {"up_in_cloud_sea_buffer", "Buffer preview before Image", cloudBufferPasses},
    {"up_in_cloud_sea_image", "Buffer + Image", cloudImagePasses},
};
}

std::span<const Effect> availableEffects() { return effects; }

const Effect* findEffect(std::string_view name) {
    const auto effectsView = availableEffects();
    const auto it = std::ranges::find(effectsView, name, &Effect::name);
    return it == effectsView.end() ? nullptr : &*it;
}

namespace {
std::string effectError(const Effect& effect, std::string_view message) {
    return "Effect '" + std::string(effect.name) + "': " + std::string(message);
}
}

std::string validateEffect(const Effect& effect, const std::filesystem::path& shaderDirectory) {
    if (effect.passes.empty()) return effectError(effect, "has no render passes.");
    bool bufferProduced = false;
    for (const PassDefinition& pass : effect.passes) {
        if (!std::filesystem::is_regular_file(shaderDirectory / pass.fragment)) return effectError(effect, "shader file is missing: " + (shaderDirectory / pass.fragment).string());
        bool usedUnits[32]{};
        for (const TextureBinding& input : pass.inputs) {
            if (input.unit < 0 || input.unit >= 32) return effectError(effect, "texture unit is outside the supported range.");
            if (usedUnits[input.unit]) return effectError(effect, "texture unit " + std::to_string(input.unit) + " is bound twice in one pass.");
            usedUnits[input.unit] = true;
            if (input.source == TextureSource::BufferOutput && !bufferProduced) return effectError(effect, "BufferOutput is read before a buffer pass.");
        }
        bufferProduced = bufferProduced || pass.target == RenderTarget::Buffer;
    }
    if (effect.passes.back().target != RenderTarget::Window) return effectError(effect, "the final pass must render to the window.");
    return {};
}

std::vector<std::string> validateEffectRegistry(const std::filesystem::path& shaderDirectory) {
    std::vector<std::string> errors;
    const std::filesystem::path vertexShader = shaderDirectory / "fullscreen.vert";
    if (!std::filesystem::is_regular_file(vertexShader)) errors.push_back("Shared vertex shader is missing: " + vertexShader.string());
    for (const Effect& effect : effects) {
        if (std::string error = validateEffect(effect, shaderDirectory); !error.empty()) errors.push_back(std::move(error));
    }
    return errors;
}
