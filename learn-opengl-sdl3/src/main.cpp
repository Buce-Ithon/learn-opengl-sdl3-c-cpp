#define SDL_MAIN_USE_CALLBACKS
#include "SDL3/SDL_main.h"
#include "SDL3/SDL_init.h"

#include "app.h"

extern "C" SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    auto *app = new App();
    *appstate = app;
    return app->init(argc, argv);
}

extern "C" SDL_AppResult SDL_AppIterate(void *appstate) {
    return static_cast<App*>(appstate)->iterate();
}

extern "C" SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    return static_cast<App*>(appstate)->event(event);
}

extern "C" void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    auto *app = static_cast<App*>(appstate);
    if (!app) return;
    app->quit(result);
    delete app;
}