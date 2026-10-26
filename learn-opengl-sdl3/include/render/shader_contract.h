#pragma once

#include <array>
#include <string_view>

namespace shader_uniform {
inline constexpr std::string_view resolution = "uResolution";
inline constexpr std::string_view time = "uTime";
inline constexpr std::string_view mouse = "uMouse";
inline constexpr std::string_view shaderToyResolution = "iResolution";
inline constexpr std::string_view shaderToyTime = "iTime";
inline constexpr std::string_view moonZoom = "uZoom";
inline constexpr std::string_view moonViewCenter = "uViewCenter";
inline constexpr std::string_view moon = "uMoon";
inline constexpr std::string_view moonRipples = "uRipples";

inline constexpr std::array<std::string_view, 12> reservedUniforms = {
    resolution, time, mouse, shaderToyResolution, shaderToyTime,
    moonZoom, moonViewCenter, moon, moonRipples,
    "iChannel0", "iChannel1", "iChannelResolution",
};

// Framework-owned uniforms (resolution/time/input textures/interaction state) must not
// be exposed as user-tunable parameters, otherwise the renderer and the HUD fight over them.
[[nodiscard]] inline bool isReserved(std::string_view name) {
    if (name.starts_with("iChannel")) return true;
    for (const std::string_view reserved : reservedUniforms) {
        if (reserved == name) return true;
    }
    return false;
}
}
