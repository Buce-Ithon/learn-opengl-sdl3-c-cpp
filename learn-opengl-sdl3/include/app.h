#pragma once

#include "SDL3/SDL_init.h"

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
    bool          running_   = true;
};