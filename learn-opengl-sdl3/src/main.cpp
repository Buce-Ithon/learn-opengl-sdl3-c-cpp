#define SDL_MAIN_USE_CALLBACKS
#include "SDL3/SDL_main.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_timer.h"

#include "mouse_event.h"

static SDL_Window *window = nullptr;
static SDL_Renderer *renderer = nullptr;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    SDL_SetAppMetadata("Learn OpenGL SDL3", "1.0", "io.github.buce-ithon.learnopenglsdl3");
    
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    if (!SDL_CreateWindowAndRenderer("learn-opengl-sdl3", 800, 600, SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_OPENGL, &window, &renderer)) {
        SDL_Log("Failed to create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer,800, 600, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    
    if (!SDL_SetWindowHitTest(window, HitTestCallback, nullptr)) {
        SDL_Log("Hit testing is not support on this platform: %s", SDL_GetError());
    }
    
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    const double now  = (static_cast<double>(SDL_GetTicks())) / 1000.0;
    const float red   = static_cast<float>(0.5 + 0.5 * SDL_sin(now));
    const float green = static_cast<float>(0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 2 / 3));
    const float blue  = static_cast<float>(0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 4 / 3));
    SDL_SetRenderDrawColorFloat(renderer, red, green, blue, SDL_ALPHA_OPAQUE_FLOAT);
    SDL_RenderClear(renderer);
    
    SDL_SetRenderDrawColorFloat(renderer, blue, red, green, SDL_ALPHA_OPAQUE_FLOAT);
    SDL_RenderPresent(renderer);
    
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    else if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_ESCAPE) {
            return SDL_APP_SUCCESS;
        }
        else if (event->key.key == SDLK_Q && (event->key.mod & SDL_KMOD_CTRL)) {
            return SDL_APP_SUCCESS;
        }
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {}