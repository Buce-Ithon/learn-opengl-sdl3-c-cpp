#pragma once

#include <array>

#include <glm/glm.hpp>

inline constexpr int moonRippleCount = 8;
inline constexpr float moonRadius = 0.155f;

struct MoonInteraction {
    float zoom = 1.0f;
    glm::vec2 viewCenter{0.0f};
    glm::vec3 moon{0.0f, 0.0f, moonRadius};
    float moonStretch = 0.0f;
    float moonVelocity = 0.0f;
    float frameTime = 0.0f;
    std::array<glm::vec4, moonRippleCount> ripples{};
    int nextRipple = 0;
};

class MoonController {
public:
    void update(float aspect, float time, float deltaTime);
    void zoomAt(glm::vec2 anchor, float wheelDelta);
    void resetView();
    void click(glm::vec2 point, float time);

    [[nodiscard]] const MoonInteraction& state() const noexcept { return state_; }
    [[nodiscard]] glm::vec2 scenePosition(glm::vec2 cursor, glm::vec2 windowSize, glm::vec2 framebufferSize) const;
    [[nodiscard]] bool contains(glm::vec2 point) const;
    [[nodiscard]] float horizonHeight(float x, float time) const;

private:
    MoonInteraction state_{};
};
