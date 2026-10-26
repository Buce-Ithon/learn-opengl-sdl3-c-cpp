#include "core/app.h"

#include "app/launch_options.h"

#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"
#include "core/mouse_event.h"
#include "glad/glad.h"
#include "platform/asset_locator.h"

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {
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
    LaunchOptions options;
    try {
        options = parseLaunchOptions(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return SDL_APP_FAILURE;
    }
    if (options.listEffects) {
        for (const Effect& effect : availableEffects()) std::cout << effect.name << " " << effect.description << '\n';
        return SDL_APP_SUCCESS;
    }
    selectedEffect_ = options.effect;

    std::filesystem::path shaderPath;
    try {
        shaderPath = shaderDirectory();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return SDL_APP_FAILURE;
    }
    const std::vector<std::string> validationErrors = validateEffectRegistry(shaderPath);
    if (!validationErrors.empty()) {
        for (const std::string& error : validationErrors) std::cerr << error << '\n';
        return SDL_APP_FAILURE;
    }

    if (!platform_.initialize("learn-opengl-sdl3", 800, 600)) {
        SDL_Log("Failed to initialize SDL/OpenGL: %s", platform_.error().c_str());
        return SDL_APP_FAILURE;
    }
    if (!SDL_SetWindowHitTest(platform_.window(), HitTestCallback, nullptr)) SDL_Log("Hit testing is not supported on this platform: %s", SDL_GetError());
    try {
        renderer_.initialize(*selectedEffect_, shaderPath);
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
    if (!SDL_GetWindowSizeInPixels(platform_.window(), &width, &height) || width <= 0 || height <= 0) return SDL_APP_CONTINUE;

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

    const FrameUniforms frame{framebufferSize, shaderMousePosition(platform_.window(), framebufferSize), time};
    if (!renderer_.render(frame, moon)) {
        SDL_Log("OpenGL rendering failed.");
        return SDL_APP_FAILURE;
    }
    platform_.swap();
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
    renderer_.shutdown();
    platform_.shutdown();
}

bool App::cursorScenePosition(glm::vec2& point) const {
    int windowWidth = 0;
    int windowHeight = 0;
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    if (!SDL_GetWindowSize(platform_.window(), &windowWidth, &windowHeight) || !SDL_GetWindowSizeInPixels(platform_.window(), &framebufferWidth, &framebufferHeight) || windowWidth <= 0 || windowHeight <= 0 || framebufferWidth <= 0 || framebufferHeight <= 0) return false;
    SDL_GetMouseState(&mouseX, &mouseY);
    if (mouseX < 0.0f || mouseY < 0.0f || mouseX >= windowWidth || mouseY >= windowHeight) return false;
    point = moonController_.scenePosition({mouseX, mouseY}, {static_cast<float>(windowWidth), static_cast<float>(windowHeight)}, {static_cast<float>(framebufferWidth), static_cast<float>(framebufferHeight)});
    return true;
}
