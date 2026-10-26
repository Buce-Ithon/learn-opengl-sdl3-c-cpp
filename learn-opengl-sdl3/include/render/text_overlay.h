#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

// Minimal immediate-mode text and rectangle overlay drawn in framebuffer pixels.
// Coordinates use a top-left origin and are expected to be in framebuffer space.
class TextOverlay {
public:
    TextOverlay() = default;
    ~TextOverlay();
    TextOverlay(const TextOverlay&) = delete;
    TextOverlay& operator=(const TextOverlay&) = delete;

    [[nodiscard]] bool initialize(std::string& error);
    void shutdown();

    void begin(int framebufferWidth, int framebufferHeight);
    void drawRect(float x, float y, float width, float height, const glm::vec4& color);
    void drawText(float x, float y, float scale, const glm::vec4& color, std::string_view text);
    void end();

    [[nodiscard]] static float textWidth(std::string_view text, float scale) { return static_cast<float>(text.size()) * 8.0f * scale; }
    [[nodiscard]] static float lineHeight(float scale) { return 10.0f * scale; }

private:
    struct Vertex {
        float x;
        float y;
        float u;
        float v;
        float r;
        float g;
        float b;
        float a;
    };

    GLuint program_ = 0;
    GLuint vertexArray_ = 0;
    GLuint vertexBuffer_ = 0;
    GLuint fontTexture_ = 0;
    GLuint whiteTexture_ = 0;
    GLint viewportLocation_ = -1;
    GLint textureLocation_ = -1;
    int viewportWidth_ = 0;
    int viewportHeight_ = 0;
    std::vector<Vertex> rectVertices_;
    std::vector<Vertex> textVertices_;

    void drawBatch(GLuint texture, const std::vector<Vertex>& vertices) const;
};
