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

#include <algorithm>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <iostream>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
const glm::vec4 hudTextColor{1.0f, 1.0f, 1.0f, 1.0f};
const glm::vec4 hudDimColor{0.72f, 0.78f, 0.86f, 1.0f};
const glm::vec4 hudAccentColor{1.0f, 0.86f, 0.35f, 1.0f};
const glm::vec4 hudErrorColor{1.0f, 0.45f, 0.45f, 1.0f};

struct HudLine {
    std::string text;
    glm::vec4 color;
};

std::string singleLine(std::string_view text) {
    std::string result(text);
    for (char& character : result) {
        const unsigned char value = static_cast<unsigned char>(character);
        if (character == '\t') character = ' ';
        else if (value < 32 || value > 126) character = '?';
    }
    return result;
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
    shaderDirectory_ = shaderPath;

    if (!platform_.initialize("learn-opengl-sdl3", 800, 600)) {
        SDL_Log("Failed to initialize SDL/OpenGL: %s", platform_.error().c_str());
        return SDL_APP_FAILURE;
    }
    if (!SDL_SetWindowHitTest(platform_.window(), HitTestCallback, nullptr)) SDL_Log("Hit testing is not supported on this platform: %s", SDL_GetError());

    std::string overlayError;
    overlayReady_ = overlay_.initialize(overlayError);
    if (!overlayReady_) {
        hudVisible_ = false;
        SDL_Log("HUD disabled: %s", overlayError.c_str());
    }

    try {
        renderer_.initialize(*selectedEffect_, shaderDirectory_);
    } catch (const std::exception& error) {
        SDL_Log("Failed to initialize effect renderer: %s", error.what());
        return SDL_APP_FAILURE;
    }

    const std::span<const Effect> effects = availableEffects();
    for (std::size_t index = 0; index < effects.size(); ++index) {
        if (&effects[index] == selectedEffect_) { effectIndex_ = index; break; }
    }
    statusMessage_ = "Ready. Press / for controls.";
    updateWindowTitle();

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
    if (hudVisible_ && overlayReady_) drawHud(width, height);
    platform_.swap();
    return SDL_APP_CONTINUE;
}

SDL_AppResult App::event(SDL_Event* event) {
    switch (event->type) {
    case SDL_EVENT_QUIT: return SDL_APP_SUCCESS;
    case SDL_EVENT_KEY_DOWN:
        if (event->key.key == SDLK_ESCAPE || (event->key.key == SDLK_Q && (event->key.mod & SDL_KMOD_CTRL))) return SDL_APP_SUCCESS;
        handleRuntimeKey(event->key);
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
    overlay_.shutdown();
    renderer_.shutdown();
    platform_.shutdown();
}

void App::handleRuntimeKey(const SDL_KeyboardEvent& key) {
    switch (key.key) {
    case SDLK_SLASH:
        if (!key.repeat) hudVisible_ = !hudVisible_;
        break;
    case SDLK_R:
        if (!key.repeat) reloadShaders();
        break;
    case SDLK_LEFTBRACKET:
        if (!key.repeat) switchEffect(-1);
        break;
    case SDLK_RIGHTBRACKET:
        if (!key.repeat) switchEffect(1);
        break;
    case SDLK_P:
        renderer_.parameters().selectNext((key.mod & SDL_KMOD_SHIFT) ? -1 : 1);
        break;
    case SDLK_COMMA:
        (void)renderer_.parameters().adjustSelected(-1.0f);
        break;
    case SDLK_PERIOD:
        (void)renderer_.parameters().adjustSelected(1.0f);
        break;
    case SDLK_BACKSPACE:
        if (!key.repeat) (void)renderer_.parameters().resetSelected();
        break;
    default:
        break;
    }
}

bool App::switchEffect(int direction) {
    const std::span<const Effect> effects = availableEffects();
    if (effects.empty()) return false;
    const std::size_t nextIndex = direction >= 0 ? (effectIndex_ + 1) % effects.size() : (effectIndex_ + effects.size() - 1) % effects.size();

    std::string error;
    if (!renderer_.load(effects[nextIndex], shaderDirectory_, error)) {
        statusMessage_ = "Switch failed; keeping " + std::string(selectedEffect_->name);
        statusDetail_ = error;
        std::cerr << error << '\n';
        return false;
    }

    selectedEffect_ = &effects[nextIndex];
    effectIndex_ = nextIndex;
    moonController_ = MoonController{};
    startTicks_ = SDL_GetTicks();
    previousFrameTicks_ = startTicks_;
    statusMessage_ = "Loaded " + std::string(selectedEffect_->name);
    statusDetail_.clear();
    updateWindowTitle();
    return true;
}

bool App::reloadShaders() {
    std::string error;
    if (!renderer_.reload(shaderDirectory_, error)) {
        statusMessage_ = "Reload failed; previous shaders are still active";
        statusDetail_ = error;
        std::cerr << error << '\n';
        return false;
    }
    statusMessage_ = "Reloaded " + std::string(selectedEffect_->name);
    statusDetail_.clear();
    return true;
}

void App::updateWindowTitle() const {
    if (platform_.window() == nullptr || selectedEffect_ == nullptr) return;
    const std::string title = "learn-opengl-sdl3 - " + std::string(selectedEffect_->name) + " (" + std::to_string(effectIndex_ + 1) + "/" + std::to_string(availableEffects().size()) + ")";
    SDL_SetWindowTitle(platform_.window(), title.c_str());
}

void App::drawHud(int framebufferWidth, int framebufferHeight) {
    const float scale = framebufferWidth >= 640 ? 2.0f : 1.0f;
    const float lineHeight = TextOverlay::lineHeight(scale);
    std::vector<HudLine> lines;

    lines.push_back({std::string(selectedEffect_->name) + "   " + std::to_string(effectIndex_ + 1) + "/" + std::to_string(availableEffects().size()), hudAccentColor});

    const ParameterSet& parameters = renderer_.parameters();
    if (parameters.empty()) {
        lines.push_back({"no tunable parameters", hudDimColor});
    } else {
        constexpr std::size_t maxVisible = 8;
        const std::size_t count = parameters.parameters().size();
        const std::size_t visible = std::min(count, maxVisible);
        std::size_t start = 0;
        if (count > visible) {
            const std::size_t selected = parameters.selectedIndex();
            start = selected > visible / 2 ? selected - visible / 2 : 0;
            start = std::min(start, count - visible);
        }
        for (std::size_t index = start; index < start + visible; ++index) {
            const Parameter& parameter = parameters.parameters()[index];
            const bool selected = index == parameters.selectedIndex();
            std::string text = (selected ? "> " : "  ") + parameter.label + ": " + ParameterSet::valueText(parameter);
            if (parameter.reflected) text += " *";
            lines.push_back({std::move(text), selected ? hudAccentColor : hudTextColor});
        }
    }

    lines.push_back({"", hudTextColor});
    lines.push_back({"[ ] effect   P param   , . adjust", hudDimColor});
    lines.push_back({"Bksp reset   R reload   / hide HUD", hudDimColor});
    if (!statusMessage_.empty()) lines.push_back({statusMessage_, statusDetail_.empty() ? hudTextColor : hudErrorColor});
    if (!statusDetail_.empty()) {
        std::istringstream stream(statusDetail_);
        std::string line;
        for (int shown = 0; shown < 4 && std::getline(stream, line); ++shown) lines.push_back({std::move(line), hudErrorColor});
    }

    const int maxCharacters = std::max(8, static_cast<int>((static_cast<float>(framebufferWidth) - 32.0f) / (8.0f * scale)));
    std::size_t longest = 0;
    for (HudLine& line : lines) {
        line.text = singleLine(line.text);
        if (static_cast<int>(line.text.size()) > maxCharacters) line.text.resize(static_cast<std::size_t>(maxCharacters));
        longest = std::max(longest, line.text.size());
    }

    const float panelWidth = std::min(static_cast<float>(framebufferWidth) - 16.0f, static_cast<float>(longest) * 8.0f * scale + 16.0f);
    const float panelHeight = static_cast<float>(lines.size()) * lineHeight + 16.0f;

    overlay_.begin(framebufferWidth, framebufferHeight);
    overlay_.drawRect(8.0f, 8.0f, panelWidth, panelHeight, glm::vec4(0.0f, 0.0f, 0.0f, 0.55f));
    float y = 16.0f;
    for (const HudLine& line : lines) {
        if (!line.text.empty()) overlay_.drawText(16.0f, y, scale, line.color, line.text);
        y += lineHeight;
    }
    overlay_.end();
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
