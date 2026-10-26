#pragma once

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "effects/effect.h"
#include "effects/moon_controller.h"
#include "platform/sdl_opengl_window.h"
#include "render/effect_renderer.h"
#include "render/text_overlay.h"

#include <cstddef>
#include <filesystem>
#include <string>

class App {
public:
    App()  = default;
    ~App() = default;

    // 4 trampoline invokes
    SDL_AppResult init(int argc, char** argv);
    SDL_AppResult iterate();
    SDL_AppResult event(SDL_Event* event);
    void          quit(SDL_AppResult result);

private:
    SdlOpenGlWindow platform_;
    EffectRenderer renderer_;
    TextOverlay overlay_;
    std::filesystem::path shaderDirectory_;
    Uint64 startTicks_ = 0;

    const Effect* selectedEffect_ = nullptr;
    std::size_t effectIndex_ = 0;

    MoonController moonController_;
    Uint64 previousFrameTicks_ = 0;
    bool cursorScenePosition(glm::vec2& point) const;

    bool overlayReady_ = false;
    bool hudVisible_ = true;
    std::string statusMessage_;
    std::string statusDetail_;

    void handleRuntimeKey(const SDL_KeyboardEvent& key);
    bool switchEffect(int direction);
    bool reloadShaders();
    void updateWindowTitle() const;
    void drawHud(int framebufferWidth, int framebufferHeight);
};
