#include "platform/sdl_opengl_window.h"

#include "SDL3/SDL_error.h"
#include "SDL3/SDL_init.h"
#include "glad/glad.h"

SdlOpenGlWindow::~SdlOpenGlWindow() { shutdown(); }

bool SdlOpenGlWindow::initialize(std::string_view title, int width, int height) {
    shutdown();
    SDL_SetAppMetadata("Learn OpenGL SDL3", "1.0", "io.github.buce-ithon.learnopenglsdl3");
    const bool alreadyInitialized = SDL_WasInit(0) != 0;
    if (!SDL_Init(SDL_INIT_VIDEO)) { error_ = SDL_GetError(); return false; }
    sdlInitialized_ = true;
    ownsSdl_ = !alreadyInitialized;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    window_ = SDL_CreateWindow(title.data(), width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS | SDL_WINDOW_OPENGL);
    if (window_ == nullptr) { error_ = SDL_GetError(); shutdown(); return false; }
    context_ = SDL_GL_CreateContext(window_);
    if (context_ == nullptr || !gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress))) { error_ = SDL_GetError(); shutdown(); return false; }
    return true;
}

void SdlOpenGlWindow::shutdown() {
    if (context_ != nullptr) { SDL_GL_DestroyContext(context_); context_ = nullptr; }
    if (window_ != nullptr) { SDL_DestroyWindow(window_); window_ = nullptr; }
    if (sdlInitialized_) {
        // SDL_Quit() force-closes every subsystem regardless of its refcount, so only
        // call it when this object brought SDL up; otherwise release our own reference.
        if (ownsSdl_) SDL_Quit();
        else SDL_QuitSubSystem(SDL_INIT_VIDEO);
        sdlInitialized_ = false;
        ownsSdl_ = false;
    }
}

void SdlOpenGlWindow::swap() const { SDL_GL_SwapWindow(window_); }
