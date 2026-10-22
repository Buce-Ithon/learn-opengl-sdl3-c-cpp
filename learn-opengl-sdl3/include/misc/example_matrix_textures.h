#pragma once

#include <glad/glad.h>
#include <array>
#include <cstdint>
#include <vector>

namespace matrix {
    constexpr int textureSize = 256;
    
    // A self-contained substitute for Shadertoy's 16 x 16 character atlas.
    // Each 16 x 16 tile contains one centered 5 x 7 hexadecimal glyph.
    inline std::array<GLuint, 2> createTextures() {
        constexpr unsigned char glyphs[16][7] = {
            {14, 17, 19, 21, 25, 17, 14}, // 0
            {4, 12, 4, 4, 4, 4, 14},     // 1
            {14, 17, 1, 2, 4, 8, 31},    // 2
            {30, 1, 1, 14, 1, 1, 30},    // 3
            {2, 6, 10, 18, 31, 2, 2},    // 4
            {31, 16, 16, 30, 1, 1, 30},  // 5
            {14, 16, 16, 30, 17, 17, 14},// 6
            {31, 1, 2, 4, 8, 8, 8},     // 7
            {14, 17, 17, 14, 17, 17, 14},// 8
            {14, 17, 17, 15, 1, 1, 14}, // 9
            {14, 17, 17, 31, 17, 17, 17},// A
            {30, 17, 17, 30, 17, 17, 30},// B
            {14, 17, 16, 16, 16, 17, 14},// C
            {30, 17, 17, 17, 17, 17, 30},// D
            {31, 16, 16, 30, 16, 16, 31},// E
            {31, 16, 16, 30, 16, 16, 16} // F
        };
        std::vector<unsigned char> atlas(textureSize * textureSize, 0);
        for (int tileY = 0; tileY < 16; ++tileY)
            for (int tileX = 0; tileX < 16; ++tileX)
                for (int row = 0; row < 7; ++row)
                    for (int col = 0; col < 5; ++col)
                        if (glyphs[(tileX + tileY * 7) % 16][row] & (1 << (4 - col)))
                            for (int dy = 0; dy < 2; ++dy)
                                for (int dx = 0; dx < 2; ++dx)
                                {
                                    const int x = tileX * 16 + 3 + col * 2 + dx;
                                    const int y = tileY * 16 + 1 + (6 - row) * 2 + dy;
                                    atlas[y * textureSize + x] = 255;
                                }

        std::vector<unsigned char> noise(textureSize * textureSize * 2);
        std::uint32_t seed = 0x12345678u;
        for (auto& value : noise)
        {
            seed ^= seed << 13;
            seed ^= seed >> 17;
            seed ^= seed << 5;
            value = static_cast<unsigned char>(seed >> 24);
        }

        std::array<GLuint, 2> textures{};
        glGenTextures(2, textures.data());
        for (int channel = 0; channel < 2; ++channel)
        {
            glActiveTexture(GL_TEXTURE0 + channel);
            glBindTexture(GL_TEXTURE_2D, textures[channel]);
            // Repeat is essential: the shader negates the atlas's x coordinate
            // and scrolls the noise coordinates beyond the [0, 1] interval.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, channel == 0 ? GL_R8 : GL_RG8,
                textureSize, textureSize, 0, channel == 0 ? GL_RED : GL_RG,
                GL_UNSIGNED_BYTE, channel == 0 ? atlas.data() : noise.data());
        }
        glActiveTexture(GL_TEXTURE0);
        return textures;
    }
}