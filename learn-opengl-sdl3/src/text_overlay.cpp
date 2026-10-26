#include "render/text_overlay.h"

#include "render/debug_font_data.h"

#include <cstddef>
#include <cstdint>

namespace {
constexpr int glyphCellSize = 8;
constexpr int glyphsPerRow = 16;
constexpr int atlasWidth = glyphsPerRow * glyphCellSize;
constexpr int atlasHeight = ((render::debug_font::glyphCount + glyphsPerRow - 1) / glyphsPerRow) * glyphCellSize;

constexpr const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;
uniform vec2 uViewport;
out vec2 vTexCoord;
out vec4 vColor;
void main() {
    vec2 ndc = aPosition / uViewport * 2.0 - 1.0;
    gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);
    vTexCoord = aTexCoord;
    vColor = aColor;
}
)";

constexpr const char* fragmentShaderSource = R"(
#version 330 core
in vec2 vTexCoord;
in vec4 vColor;
uniform sampler2D uTexture;
out vec4 FragColor;
void main() {
    float coverage = texture(uTexture, vTexCoord).r;
    FragColor = vec4(vColor.rgb, vColor.a * coverage);
}
)";

GLuint compileShader(GLenum type, const char* source, std::string& error) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) return shader;

    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::string log(static_cast<std::size_t>(length), '\0');
    glGetShaderInfoLog(shader, length, nullptr, log.data());
    glDeleteShader(shader);
    error = "Unable to compile HUD shader:\n" + log;
    return 0;
}

GLuint createProgram(std::string& error) {
    const GLuint vertex = compileShader(GL_VERTEX_SHADER, vertexShaderSource, error);
    if (vertex == 0) return 0;
    const GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource, error);
    if (fragment == 0) {
        glDeleteShader(vertex);
        return 0;
    }
    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE) return program;

    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    std::string log(static_cast<std::size_t>(length), '\0');
    glGetProgramInfoLog(program, length, nullptr, log.data());
    glDeleteProgram(program);
    error = "Unable to link HUD shader:\n" + log;
    return 0;
}

GLuint createFontTexture() {
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(atlasWidth) * atlasHeight, 0);
    for (int glyph = 0; glyph < render::debug_font::glyphCount; ++glyph) {
        const int cellX = (glyph % glyphsPerRow) * glyphCellSize;
        const int cellY = (glyph / glyphsPerRow) * glyphCellSize;
        for (int row = 0; row < glyphCellSize; ++row) {
            const std::uint8_t bits = render::debug_font::glyphs[glyph * 8 + row];
            for (int column = 0; column < glyphCellSize; ++column) {
                if ((bits >> column) & 1u) pixels[static_cast<std::size_t>(cellY + row) * atlasWidth + cellX + column] = 255;
            }
        }
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlasWidth, atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return texture;
}

GLuint createWhiteTexture() {
    constexpr std::uint8_t white = 255;
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, &white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return texture;
}

int glyphIndex(unsigned char character) {
    if (character < 33 || character > 126) return render::debug_font::glyphCount - 1;
    return character - 33;
}
}

TextOverlay::~TextOverlay() { shutdown(); }

bool TextOverlay::initialize(std::string& error) {
    shutdown();
    program_ = createProgram(error);
    if (program_ == 0) return false;
    viewportLocation_ = glGetUniformLocation(program_, "uViewport");
    textureLocation_ = glGetUniformLocation(program_, "uTexture");

    glGenVertexArrays(1, &vertexArray_);
    glGenBuffers(1, &vertexBuffer_);
    glBindVertexArray(vertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, x)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, u)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, r)));
    glBindVertexArray(0);

    fontTexture_ = createFontTexture();
    whiteTexture_ = createWhiteTexture();
    return fontTexture_ != 0 && whiteTexture_ != 0;
}

void TextOverlay::shutdown() {
    rectVertices_.clear();
    textVertices_.clear();
    if (vertexBuffer_ != 0) { glDeleteBuffers(1, &vertexBuffer_); vertexBuffer_ = 0; }
    if (vertexArray_ != 0) { glDeleteVertexArrays(1, &vertexArray_); vertexArray_ = 0; }
    if (fontTexture_ != 0) { glDeleteTextures(1, &fontTexture_); fontTexture_ = 0; }
    if (whiteTexture_ != 0) { glDeleteTextures(1, &whiteTexture_); whiteTexture_ = 0; }
    if (program_ != 0) { glDeleteProgram(program_); program_ = 0; }
}

void TextOverlay::begin(int framebufferWidth, int framebufferHeight) {
    viewportWidth_ = framebufferWidth;
    viewportHeight_ = framebufferHeight;
    rectVertices_.clear();
    textVertices_.clear();
}

void TextOverlay::drawRect(float x, float y, float width, float height, const glm::vec4& color) {
    const Vertex topLeft{x, y, 0.5f, 0.5f, color.r, color.g, color.b, color.a};
    const Vertex bottomLeft{x, y + height, 0.5f, 0.5f, color.r, color.g, color.b, color.a};
    const Vertex topRight{x + width, y, 0.5f, 0.5f, color.r, color.g, color.b, color.a};
    const Vertex bottomRight{x + width, y + height, 0.5f, 0.5f, color.r, color.g, color.b, color.a};
    rectVertices_.insert(rectVertices_.end(), {topLeft, bottomLeft, topRight, topRight, bottomLeft, bottomRight});
}

void TextOverlay::drawText(float x, float y, float scale, const glm::vec4& color, std::string_view text) {
    const float glyphWidth = glyphCellSize * scale;
    const float glyphHeight = glyphCellSize * scale;
    for (const char character : text) {
        if (character == ' ') { x += glyphWidth; continue; }
        const int glyph = glyphIndex(static_cast<unsigned char>(character));
        const float u0 = static_cast<float>((glyph % glyphsPerRow) * glyphCellSize) / atlasWidth;
        const float v0 = static_cast<float>((glyph / glyphsPerRow) * glyphCellSize) / atlasHeight;
        const float u1 = u0 + static_cast<float>(glyphCellSize) / atlasWidth;
        const float v1 = v0 + static_cast<float>(glyphCellSize) / atlasHeight;

        const Vertex topLeft{x, y, u0, v0, color.r, color.g, color.b, color.a};
        const Vertex bottomLeft{x, y + glyphHeight, u0, v1, color.r, color.g, color.b, color.a};
        const Vertex topRight{x + glyphWidth, y, u1, v0, color.r, color.g, color.b, color.a};
        const Vertex bottomRight{x + glyphWidth, y + glyphHeight, u1, v1, color.r, color.g, color.b, color.a};
        textVertices_.insert(textVertices_.end(), {topLeft, bottomLeft, topRight, topRight, bottomLeft, bottomRight});
        x += glyphWidth;
    }
}

void TextOverlay::end() {
    if (viewportWidth_ <= 0 || viewportHeight_ <= 0) return;
    glViewport(0, 0, viewportWidth_, viewportHeight_);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawBatch(whiteTexture_, rectVertices_);
    drawBatch(fontTexture_, textVertices_);
    glDisable(GL_BLEND);
    rectVertices_.clear();
    textVertices_.clear();
}

void TextOverlay::drawBatch(GLuint texture, const std::vector<Vertex>& vertices) const {
    if (vertices.empty()) return;
    glUseProgram(program_);
    glUniform2f(viewportLocation_, static_cast<float>(viewportWidth_), static_cast<float>(viewportHeight_));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(textureLocation_, 0);
    glBindVertexArray(vertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);
}
