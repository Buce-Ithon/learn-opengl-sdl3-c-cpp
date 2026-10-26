#pragma once

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
}
