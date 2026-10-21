#include "app.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_video.h"
#include "mouse_event.h"
#include "shader.h"
#include "glad/glad.h"

SDL_AppResult App::init(int /*argc*/, char** /*argv*/) {
    SDL_SetAppMetadata("Learn OpenGL SDL3", "1.0", "io.github.buce-ithon.learnopenglsdl3");
    
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    window_ = SDL_CreateWindow("learn-opengl-sdl3", 800, 600, SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_OPENGL);
    if (!window_) {
        SDL_Log("Failed to create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    if (!SDL_SetWindowHitTest(window_, HitTestCallback, nullptr)) {
        SDL_Log("Hit testing is not support on this platform: %s", SDL_GetError());
    }
    
    glContext_ = SDL_GL_CreateContext(window_);
    if (!glContext_) {
        SDL_Log("Failed to create/bind GL context: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    // Sync viewport manually
    int fbW, fbH;
    SDL_GetWindowSizeInPixels(window_, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);
    
    return SDL_APP_CONTINUE;
}

SDL_AppResult App::iterate() { 
    if (!running_) return SDL_APP_SUCCESS;
    
    // TODO every frame render actions
    // ...
    
    if (window_) SDL_GL_SwapWindow(window_);

    return SDL_APP_CONTINUE;
}

SDL_AppResult App::event(SDL_Event *event) { 
    switch (event->type) {
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;
    case SDL_EVENT_KEY_DOWN:
        if (event->key.key == SDLK_ESCAPE) return SDL_APP_SUCCESS;
        if (event->key.key == SDLK_Q && (event->key.mod & SDL_KMOD_CTRL)) return SDL_APP_SUCCESS;
        break;
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        glViewport(0, 0, event->window.data1, event->window.data2);
    }
    return SDL_APP_CONTINUE;
}

void App::quit(SDL_AppResult /*result*/) {
    if (glContext_) {
        SDL_GL_DestroyContext(glContext_);
        glContext_ = nullptr;
    }
}
