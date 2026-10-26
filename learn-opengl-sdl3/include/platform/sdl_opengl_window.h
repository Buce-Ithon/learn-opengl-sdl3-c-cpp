#pragma once

#include "SDL3/SDL_video.h"

#include <string>
#include <string_view>

class SdlOpenGlWindow {
public:
    SdlOpenGlWindow() = default;
    ~SdlOpenGlWindow();
    SdlOpenGlWindow(const SdlOpenGlWindow&) = delete;
    SdlOpenGlWindow& operator=(const SdlOpenGlWindow&) = delete;

    [[nodiscard]] bool initialize(std::string_view title, int width, int height);
    void shutdown();
    [[nodiscard]] SDL_Window* window() const noexcept { return window_; }
    void swap() const;
    [[nodiscard]] const std::string& error() const noexcept { return error_; }

private:
    SDL_Window* window_ = nullptr;
    SDL_GLContext context_ = nullptr;
    bool sdlInitialized_ = false;
    bool ownsSdl_ = false;
    std::string error_;
};
