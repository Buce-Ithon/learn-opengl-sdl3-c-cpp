#include "render/resource_factory.h"

#include <cstdint>
#include <vector>

namespace render {
std::array<GLuint, 2> createMatrixTextures() {
    constexpr unsigned char glyphs[16][7] = {{14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},{14,17,17,15,1,1,14},{14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16}};
    std::vector<unsigned char> atlas(matrixTextureSize * matrixTextureSize, 0);
    for (int tileY = 0; tileY < 16; ++tileY) for (int tileX = 0; tileX < 16; ++tileX) for (int row = 0; row < 7; ++row) for (int col = 0; col < 5; ++col) if (glyphs[(tileX + tileY * 7) % 16][row] & (1 << (4 - col))) for (int dy = 0; dy < 2; ++dy) for (int dx = 0; dx < 2; ++dx) atlas[(tileY * 16 + 1 + (6 - row) * 2 + dy) * matrixTextureSize + tileX * 16 + 3 + col * 2 + dx] = 255;
    std::vector<unsigned char> noise(matrixTextureSize * matrixTextureSize * 2);
    std::uint32_t seed = 0x12345678u;
    for (auto& value : noise) { seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; value = static_cast<unsigned char>(seed >> 24); }
    std::array<GLuint, 2> textures{};
    glGenTextures(2, textures.data());
    for (int channel = 0; channel < 2; ++channel) {
        glBindTexture(GL_TEXTURE_2D, textures[channel]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, channel == 0 ? GL_R8 : GL_RG8, matrixTextureSize, matrixTextureSize, 0, channel == 0 ? GL_RED : GL_RG, GL_UNSIGNED_BYTE, channel == 0 ? atlas.data() : noise.data());
    }
    return textures;
}

GLuint createNoiseTexture() {
    constexpr int size = 1024;
    std::vector<unsigned char> pixels(size * size);
    for (int y = 0; y < size; ++y) for (int x = 0; x < size; ++x) { unsigned int value = static_cast<unsigned int>(x * 374761393u + y * 668265263u); value = (value ^ (value >> 13u)) * 1274126177u; pixels[y * size + x] = static_cast<unsigned char>((value ^ (value >> 16u)) & 255u); }
    GLuint texture = 0; glGenTextures(1, &texture); glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, size, size, 0, GL_RED, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return texture;
}

GLuint createWhiteTexture() {
    constexpr unsigned char white[] = {255, 255, 255, 255};
    GLuint texture = 0; glGenTextures(1, &texture); glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return texture;
}

bool createBufferTarget(int width, int height, GLuint& framebuffer, GLuint& colorTexture) {
    glGenFramebuffers(1, &framebuffer); glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glGenTextures(1, &colorTexture); glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
    const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (!complete) { glDeleteTextures(1, &colorTexture); glDeleteFramebuffers(1, &framebuffer); colorTexture = 0; framebuffer = 0; }
    return complete;
}
}
