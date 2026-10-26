#include "effects/moon_controller.h"

#include <algorithm>
#include <cmath>

void MoonController::update(float aspect, float time, float deltaTime) {
    constexpr float horizonBase = -0.12f;
    constexpr float damping = 4.5f;
    constexpr float frequency = 12.0f;

    const float a = state_.moonStretch;
    const float b = (state_.moonVelocity + damping * a) / frequency;
    const float decay = std::exp(-damping * deltaTime);
    const float cosine = std::cos(frequency * deltaTime);
    const float sine = std::sin(frequency * deltaTime);

    state_.moonStretch = decay * (a * cosine + b * sine);
    state_.moonVelocity = decay * ((b * frequency - damping * a) * cosine - (a * frequency + damping * b) * sine);
    state_.moon = {aspect * 0.22f, horizonBase + moonRadius + 0.025f + 0.23f * (1.0f - std::exp(-time * 0.018f)), moonRadius * (1.0f + state_.moonStretch)};
    state_.frameTime = time;
}

void MoonController::zoomAt(glm::vec2 anchor, float wheelDelta) {
    const float oldZoom = state_.zoom;
    state_.zoom = std::clamp(oldZoom * std::exp(wheelDelta * 0.12f), 0.65f, 3.0f);
    state_.viewCenter = anchor - (anchor - state_.viewCenter) * (oldZoom / state_.zoom);
}

void MoonController::resetView() {
    state_.zoom = 1.0f;
    state_.viewCenter = {};
}

void MoonController::click(glm::vec2 point, float time) {
    if (contains(point)) {
        state_.moonVelocity = std::min(state_.moonVelocity + 5.0f, 6.0f);
        return;
    }
    const float kind = point.y <= horizonHeight(point.x, state_.frameTime) ? 2.0f : 1.0f;
    state_.ripples[state_.nextRipple] = {point, time, kind};
    state_.nextRipple = (state_.nextRipple + 1) % moonRippleCount;
}

glm::vec2 MoonController::scenePosition(glm::vec2 cursor, glm::vec2 windowSize, glm::vec2 framebufferSize) const {
    const float aspect = framebufferSize.x / framebufferSize.y;
    const glm::vec2 screen{(2.0f * cursor.x / windowSize.x - 1.0f) * aspect, 1.0f - 2.0f * cursor.y / windowSize.y};
    return state_.viewCenter + screen / state_.zoom;
}

bool MoonController::contains(glm::vec2 point) const {
    return glm::length(point - glm::vec2(state_.moon)) <= state_.moon.z && point.y > horizonHeight(point.x, state_.frameTime);
}

float MoonController::horizonHeight(float x, float time) const {
    constexpr float horizonBase = -0.12f;
    return horizonBase + 0.009f * std::sin(x * 4.2f - time * 0.42f) + 0.004f * std::sin(x * 10.7f + time * 0.63f + 1.4f) + 0.002f * std::sin(x * 22.0f - time * 0.85f + 0.6f);
}
