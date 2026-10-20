#include <string.h>
#include <math.h>
#define SDL_MAIN_USE_CALLBACKS
#include "SDL3/SDL_main.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_timer.h"

int *gFrameBuffer;
static SDL_Window *gSDLWindow;
static SDL_Renderer *gSDLRenderer;
static SDL_Texture *gSDLTexture;
const int WINDOW_WIDTH = 1920 / 2;
const int WINDOW_HEIGHT = 1080 / 2;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    SDL_SetAppMetadata("Learn OpenGL SDL3 Test1", "1.0", "io.github.buce-ithon.learnopenglsdl3.test1");
    
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    gFrameBuffer = new int[WINDOW_WIDTH * WINDOW_HEIGHT];
    
    if (!gFrameBuffer) {
        SDL_Log("Failed to create frame buffer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    if (!SDL_CreateWindowAndRenderer("learn-opengl-sdl3-test-1", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_OPENGL, &gSDLWindow, &gSDLRenderer)) {
        SDL_Log("Failed to create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    gSDLTexture = SDL_CreateTexture(gSDLRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT); 
    
    if (!gSDLTexture) {
        SDL_Log("Failed to create texture: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    return SDL_APP_CONTINUE;
}

bool update()
{
  char* pix;
  int pitch;
  
  SDL_LockTexture(gSDLTexture, nullptr, (void**)&pix, &pitch);
  for (int i = 0, sp = 0, dp = 0; i < WINDOW_HEIGHT; i++, dp += WINDOW_WIDTH, sp += pitch)
    memcpy(pix + sp, gFrameBuffer + dp, WINDOW_WIDTH * 4);

  SDL_UnlockTexture(gSDLTexture);  
  SDL_RenderTexture(gSDLRenderer, gSDLTexture, nullptr, nullptr);
  SDL_RenderPresent(gSDLRenderer);
  SDL_Delay(1);
  return true;
}

void render(Uint64 aTicks)
{
  for (int i = 0, c = 0; i < WINDOW_HEIGHT; i++)
  {
    for (int j = 0; j < WINDOW_WIDTH; j++, c++)
    {
      gFrameBuffer[c] = (int)(i * i + j * j + aTicks) | 0xff000000;
    }
  }
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    update();
    render(SDL_GetTicks());
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