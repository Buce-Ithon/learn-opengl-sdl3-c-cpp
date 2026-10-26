#pragma once

#include <glad/glad.h>

#include <utility>

class Texture2D {
public:
    Texture2D() = default;
    explicit Texture2D(GLuint id) : id_(id) {}
    ~Texture2D() { reset(); }
    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;
    Texture2D(Texture2D&& other) noexcept : id_(std::exchange(other.id_, 0)) {}
    Texture2D& operator=(Texture2D&& other) noexcept { if (this != &other) { reset(); id_ = std::exchange(other.id_, 0); } return *this; }
    [[nodiscard]] GLuint id() const noexcept { return id_; }
    void reset(GLuint id = 0) noexcept { if (id_ != 0) glDeleteTextures(1, &id_); id_ = id; }
private:
    GLuint id_ = 0;
};

class Framebuffer {
public:
    Framebuffer() = default;
    explicit Framebuffer(GLuint id) : id_(id) {}
    ~Framebuffer() { reset(); }
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    Framebuffer(Framebuffer&& other) noexcept : id_(std::exchange(other.id_, 0)) {}
    Framebuffer& operator=(Framebuffer&& other) noexcept { if (this != &other) { reset(); id_ = std::exchange(other.id_, 0); } return *this; }
    [[nodiscard]] GLuint id() const noexcept { return id_; }
    void reset(GLuint id = 0) noexcept { if (id_ != 0) glDeleteFramebuffers(1, &id_); id_ = id; }
private:
    GLuint id_ = 0;
};

class VertexArray {
public:
    VertexArray() = default;
    ~VertexArray() { reset(); }
    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;
    [[nodiscard]] GLuint id() const noexcept { return id_; }
    void create() { if (id_ == 0) glGenVertexArrays(1, &id_); }
    void reset() noexcept { if (id_ != 0) glDeleteVertexArrays(1, &id_); id_ = 0; }
private:
    GLuint id_ = 0;
};
