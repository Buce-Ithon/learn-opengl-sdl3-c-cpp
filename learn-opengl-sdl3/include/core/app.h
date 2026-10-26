#pragma once

#include "SDL3/SDL_init.h"
#include "effects/effect.h"
#include "effects/moon_controller.h"
#include "render/effect_renderer.h"

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
    SDL_Window*   window_    = nullptr;
    SDL_GLContext glContext_ = nullptr;
    EffectRenderer renderer_;
    Uint64 startTicks_ = 0;
    
    const Effect* selectedEffect_ = nullptr;
    
    MoonController moonController_;
    Uint64 previousFrameTicks_ = 0;
    bool cursorScenePosition(glm::vec2& point) const;
};
