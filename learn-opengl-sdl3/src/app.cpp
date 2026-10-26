#include "core/app.h"

#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_filesystem.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"
#include "core/mouse_event.h"
#include "glad/glad.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
std::filesystem::path shaderDirectory() {
    const char* basePath = SDL_GetBasePath();
    if (basePath == nullptr) throw std::runtime_error(std::string("Unable to determine application directory: ") + SDL_GetError());
    return std::filesystem::path(basePath) / "assets" / "shaders";
}

glm::vec2 shaderMousePosition(SDL_Window* window, glm::vec2 framebufferSize) {
    int windowWidth = 0;
    int windowHeight = 0;
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    SDL_GetMouseState(&mouseX, &mouseY);
    if (windowWidth <= 0 || windowHeight <= 0 || mouseX < 0.0f || mouseY < 0.0f || mouseX >= windowWidth || mouseY >= windowHeight) return {};
    return {mouseX / windowWidth * framebufferSize.x, (1.0f - mouseY / windowHeight) * framebufferSize.y};
}
}

SDL_AppResult App::init(int argc, char** argv) {
    selectedEffect_ = &availableEffects().front();
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--list") {
            for (const Effect& effect : availableEffects()) std::cout << effect.name << " " << effect.description << '\n';
            return SDL_APP_SUCCESS;
        }
        if (arg == "--effect") {
            if (++i == argc) {
                std::cerr << "--effect requires a name.\n";
                return SDL_APP_FAILURE;
            }
            selectedEffect_ = findEffect(argv[i]);
            if (selectedEffect_ == nullptr) {
                std::cerr << "Unknown effect: " << argv[i] << '\n';
                return SDL_APP_FAILURE;
            }
        }
    }

    SDL_SetAppMetadata("Learn OpenGL SDL3", "1.0", "io.github.buce-ithon.learnopenglsdl3");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    window_ = SDL_CreateWindow("learn-opengl-sdl3", 800, 600, SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS | SDL_WINDOW_OPENGL);
    if (window_ == nullptr) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!SDL_SetWindowHitTest(window_, HitTestCallback, nullptr)) SDL_Log("Hit testing is not supported on this platform: %s", SDL_GetError());

    glContext_ = SDL_GL_CreateContext(window_);
    if (glContext_ == nullptr || !gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
        SDL_Log("Failed to create OpenGL context: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    try {
        renderer_.initialize(*selectedEffect_, shaderDirectory());
    } catch (const std::exception& error) {
        SDL_Log("Failed to initialize effect renderer: %s", error.what());
        return SDL_APP_FAILURE;
    }

    startTicks_ = SDL_GetTicks();
    previousFrameTicks_ = startTicks_;
    return SDL_APP_CONTINUE;
}

SDL_AppResult App::iterate() {
    int width = 0;
    int height = 0;
    if (!SDL_GetWindowSizeInPixels(window_, &width, &height) || width <= 0 || height <= 0) return SDL_APP_CONTINUE;

    const Uint64 nowTicks = SDL_GetTicks();
    const float time = static_cast<float>(nowTicks - startTicks_) / 1000.0f;
    const float deltaTime = static_cast<float>(nowTicks - previousFrameTicks_) / 1000.0f;
    previousFrameTicks_ = nowTicks;
    const glm::vec2 framebufferSize{static_cast<float>(width), static_cast<float>(height)};

    const MoonInteraction* moon = nullptr;
    if (selectedEffect_->interaction == InteractionMode::Moon) {
        moonController_.update(framebufferSize.x / framebufferSize.y, time, deltaTime);
        moon = &moonController_.state();
    }

    const FrameUniforms frame{framebufferSize, shaderMousePosition(window_, framebufferSize), time};
    if (!renderer_.render(frame, moon)) {
        SDL_Log("OpenGL rendering failed.");
        return SDL_APP_FAILURE;
    }
    SDL_GL_SwapWindow(window_);
    return SDL_APP_CONTINUE;
}

SDL_AppResult App::event(SDL_Event* event) {
    switch (event->type) {
    case SDL_EVENT_QUIT: return SDL_APP_SUCCESS;
    case SDL_EVENT_KEY_DOWN:
        if (event->key.key == SDLK_ESCAPE || (event->key.key == SDLK_Q && (event->key.mod & SDL_KMOD_CTRL))) return SDL_APP_SUCCESS;
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        if (selectedEffect_->interaction == InteractionMode::Moon) {
            glm::vec2 anchor;
            if (cursorScenePosition(anchor)) moonController_.zoomAt(anchor, event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -event->wheel.y : event->wheel.y);
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (selectedEffect_->interaction != InteractionMode::Moon) break;
        if (event->button.button == SDL_BUTTON_RIGHT) moonController_.resetView();
        if (event->button.button == SDL_BUTTON_LEFT) {
            glm::vec2 point;
            if (cursorScenePosition(point)) moonController_.click(point, static_cast<float>(SDL_GetTicks() - startTicks_) / 1000.0f);
        }
        break;
    default: break;
    }
    return SDL_APP_CONTINUE;
}

void App::quit(SDL_AppResult /*result*/) {
    if (glContext_ != nullptr) {
        renderer_.shutdown();
        SDL_GL_DestroyContext(glContext_);
        glContext_ = nullptr;
    }
    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

bool App::cursorScenePosition(glm::vec2& point) const {
    int windowWidth = 0;
    int windowHeight = 0;
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    if (!SDL_GetWindowSize(window_, &windowWidth, &windowHeight) || !SDL_GetWindowSizeInPixels(window_, &framebufferWidth, &framebufferHeight) || windowWidth <= 0 || windowHeight <= 0 || framebufferWidth <= 0 || framebufferHeight <= 0) return false;
    SDL_GetMouseState(&mouseX, &mouseY);
    if (mouseX < 0.0f || mouseY < 0.0f || mouseX >= windowWidth || mouseY >= windowHeight) return false;
    point = moonController_.scenePosition({mouseX, mouseY}, {static_cast<float>(windowWidth), static_cast<float>(windowHeight)}, {static_cast<float>(framebufferWidth), static_cast<float>(framebufferHeight)});
    return true;
}
