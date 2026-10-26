#include "effects/effect.h"

#include <algorithm>

namespace {
constexpr Effect effects[] = {
    {"matrix", "matrix.frag", Pipeline::Single, "Character rain", ResourceProfile::GlyphAtlasAndNoise, InteractionMode::None},
    {"mt_fuji_watercolor", "mt_fuji_watercolor.frag", Pipeline::Single, "Mt. Fuji watercolor", ResourceProfile::None, InteractionMode::None},
    {"van_gogh_sunset", "van_gogh_sunset.frag", Pipeline::Single, "Van Gogh Sunset", ResourceProfile::None, InteractionMode::None},
    {"square_bg_v2", "square_bg_v2.frag", Pipeline::Single, "Square Background V2", ResourceProfile::None, InteractionMode::None},
    {"paper_sticks", "paper_sticks.frag", Pipeline::Single, "Paper Sticks", ResourceProfile::None, InteractionMode::None},
    {"infinity_loader", "infinity_loader.frag", Pipeline::Single, "Infinity loader", ResourceProfile::None, InteractionMode::None},
    {"octagrams", "octagrams.frag", Pipeline::Single, "Octagrams", ResourceProfile::None, InteractionMode::None},
    {"neon_infinity", "neon_infinity.frag", Pipeline::Single, "NEON INFINITY", ResourceProfile::None, InteractionMode::None},
    {"cyber_fuji_2020", "cyber_fuji_2020.frag", Pipeline::Single, "Cyber Fuji 2020", ResourceProfile::None, InteractionMode::None},
    {"pretty_hip", "pretty_hip.frag", Pipeline::Single, "Pretty Hip", ResourceProfile::None, InteractionMode::None},
    {"midmoon", "midmoon.frag", Pipeline::Single, "Interactive Moonrise", ResourceProfile::None, InteractionMode::Moon},
    {"up_in_cloud_sea_buffer", "up_in_cloud_sea_buffer.frag", Pipeline::Single, "Buffer preview before Image", ResourceProfile::NoiseAndWhite, InteractionMode::None},
    {"up_in_cloud_sea_image", "up_in_cloud_sea_image.frag", Pipeline::TwoPass, "Buffer + Image", ResourceProfile::NoiseAndWhite, InteractionMode::None},
};
}

std::span<const Effect> availableEffects() { return effects; }

const Effect* findEffect(std::string_view name) {
    const auto effectsView = availableEffects();
    const auto it = std::ranges::find(effectsView, name, &Effect::name);
    return it == effectsView.end() ? nullptr : &*it;
}
