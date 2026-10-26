#pragma once

#include <array>

#include <glad/glad.h>

namespace render {
inline constexpr int matrixTextureSize = 256;

[[nodiscard]] std::array<GLuint, 2> createMatrixTextures();
[[nodiscard]] GLuint createNoiseTexture();
[[nodiscard]] GLuint createWhiteTexture();
[[nodiscard]] bool createBufferTarget(int width, int height, GLuint& framebuffer, GLuint& colorTexture);
}
