#pragma once

#include <span>
#include <string_view>

enum class Pipeline {
    Single,
    TwoPass,
};

enum class ResourceProfile {
    None,
    GlyphAtlasAndNoise,
    NoiseAndWhite,
};

enum class InteractionMode {
    None,
    Moon,
};

struct Effect {
    std::string_view name;
    std::string_view fragment;
    Pipeline pipeline;
    std::string_view description;
    ResourceProfile resources;
    InteractionMode interaction;
};

[[nodiscard]] std::span<const Effect> availableEffects();
[[nodiscard]] const Effect* findEffect(std::string_view name);
