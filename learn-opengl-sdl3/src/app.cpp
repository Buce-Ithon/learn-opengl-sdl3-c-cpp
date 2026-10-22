#include "core/app.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_video.h"
#include "core/mouse_event.h"
#include "core/shader.h"
#include "glad/glad.h"
#include "misc/example_matrix_textures.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_mouse.h"
#include "misc/shader_test_resources.h"

#include <iostream>
#include <string>
#include <cmath>
#include <algorithm>

namespace {
    constexpr Effect effects[] = {
        {"matrix", "matrix.frag", Pipeline::Single, "Character rain", ResourceProfile::GlyphAtlasAndNoise, InteractionMode::None}, 
        {"mt_fuji_watercolor", "mt_fuji_watercolor.frag", Pipeline::Single, "Mt. Fuji watercolor", ResourceProfile::None, InteractionMode::None},
        {"van_gogh_sunset", "van_gogh_sunset.frag", Pipeline::Single, "Van Gogh Sunset", ResourceProfile::None, InteractionMode::None},
        {"square_bg_v2", "square_bg_v2.frag", Pipeline::Single, "Square Background V2", ResourceProfile::None, InteractionMode::None},
        {"paper_sticks", "paper_sticks.frag", Pipeline::Single, "Paper Sticks", ResourceProfile::None, InteractionMode::None},
        {"infinity_loader", "infinity_loader.frag", Pipeline::Single, "Infinity loader", ResourceProfile::None, InteractionMode::None},
        {"octagrams", "octagrams.frag", Pipeline::Single, "Octagrams", ResourceProfile::None, InteractionMode::None},
        {"neon_infinity", "neon_infinity.frag", Pipeline::Single, "NEON INFINITY", ResourceProfile::None, InteractionMode::None},
        {"cyber_fuji_2020", "cyber_fuji_2020.frag", Pipeline::Single, "Cyber Fuji 2020", ResourceProfile::None, InteractionMode::None},
        {"pretty_hip", "pretty_hip.frag", Pipeline::Single, "Pretty Hip", ResourceProfile::None, InteractionMode::None},
        {"midmoon", "midmoon.frag", Pipeline::Single, "Interactive Moonrise", ResourceProfile::None, InteractionMode::Moon},
        {"up_in_cloud_sea_buffer", "up_in_cloud_sea_buffer.frag", Pipeline::Single, "Buffer preview before Image", ResourceProfile::NoiseAndWhite, InteractionMode::None},
        {"up_in_cloud_sea_image", "up_in_cloud_sea_image.frag", Pipeline::TwoPass, "Buffer + Image", ResourceProfile::NoiseAndWhite, InteractionMode::None}
    };
    
    const Effect* findEffect(const std::string& name) {
        for (const auto& effect : effects) {
            if (name == effect.name) {
                return &effect;
            }
        }
        return nullptr;
    }
    
    bool isLinked(const Shader& shader) {
        GLint linked = GL_FALSE;
        glGetProgramiv(shader.ID, GL_LINK_STATUS, &linked);
        return linked;
    }
}

SDL_AppResult App::init(int argc, char** argv) {
    // Parse command-line arguments
    selectedEffect_ = &effects[0]; // default matrix.frag

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--list") {
            for (const auto& effect : effects) {
                std::cout << effect.name << " " << effect.description << '\n';
            }
            return SDL_APP_SUCCESS;
        }
        
        if (arg == "--effect") {
            if (++i == argc) {
                std::cerr << "--effect requires a name.\n";
                return SDL_APP_FAILURE;
            }
            
            selectedEffect_ = findEffect(argv[i]);
            if (!selectedEffect_) {
                std::cerr << "Unkown effect: " << argv[i] << '\n';
                return SDL_APP_FAILURE;
            }
        }
    }

    // SDL initialization
    SDL_SetAppMetadata("Learn OpenGL SDL3", "1.0", "io.github.buce-ithon.learnopenglsdl3");
    
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    
    window_ = SDL_CreateWindow("learn-opengl-sdl3", 800, 600, SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS | SDL_WINDOW_OPENGL);
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
    
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
        SDL_Log("Failed to load GLAD: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    // Load shader
    const std::string fragmentPath = std::string("./assets/shaders/") + selectedEffect_->fragment;
    shader_ = std::make_unique<Shader>("./assets/shaders/fullscreen.vert", fragmentPath.c_str());
    if (!isLinked(*shader_)) {
        SDL_Log("Failed to link the Matrix shader program: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (selectedEffect_->resources == ResourceProfile::GlyphAtlasAndNoise) {
        matrixTextures_ = matrix::createTextures();

        shader_->use();
        shader_->setInt("iChannel0", 0);
        shader_->setInt("iChannel1", 1);

        for (int i = 0; i < 2; ++i) {
            shader_->setVec3("iChannelResolution[" + std::to_string(i) + "]", static_cast<float>(matrix::textureSize), static_cast<float>(matrix::textureSize), 1.0f);
        }
    }
    if (selectedEffect_->resources == ResourceProfile::NoiseAndWhite) {
        noiseTexture_ = shader_test::createNoiseTexture();
        whiteTexture_ = shader_test::createWhiteTexture();
    }
    if (selectedEffect_->pipeline == Pipeline::TwoPass) {
        bufferShader_ = std::make_unique<Shader>("./assets/shaders/fullscreen.vert", "./assets/shaders/up_in_cloud_sea_buffer.frag");
        if (!isLinked(*bufferShader_)) {
            SDL_Log("Failed to link up_to_cloud_sea Buffer shader program: %s", SDL_GetError());
            return SDL_APP_FAILURE;
        }
    }
    glGenVertexArrays(1, &vertexArray_);
    startTicks_ = SDL_GetTicks();
    previousFrameTicks_ = startTicks_;
    moonInteraction_.ripples.fill(glm::vec4(0.0f));
    
    // Sync viewport manually
    int fbW, fbH;
    SDL_GetWindowSizeInPixels(window_, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);
    
    return SDL_APP_CONTINUE;
}

SDL_AppResult App::iterate() { 
    int width  = 0;
    int height = 0;
    if (!SDL_GetWindowSizeInPixels(window_, &width, &height) || width <= 0 || height <= 0) {
        return SDL_APP_CONTINUE;
    }
    
    const Uint64 nowTicks = SDL_GetTicks();
    const float time = static_cast<float>(nowTicks - startTicks_) / 1000.0f;
    const float deltaTime = static_cast<float>(nowTicks - previousFrameTicks_) / 1000.0f;
    previousFrameTicks_ = nowTicks;
    
    if (selectedEffect_->interaction == InteractionMode::Moon) {
        updateMoon(static_cast<float>(width) / static_cast<float>(height), time, deltaTime);
    }
    
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glBindVertexArray(vertexArray_);

    // Pass 1: Only up_in_cloud_sea_image need it.
    // Buffer shader render to an off-screen color texture, rather than directly to the window.
    if (selectedEffect_->pipeline == Pipeline::TwoPass) {
        if (!ensureBufferTarget(width, height)) {
            return SDL_APP_FAILURE;
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, bufferFramebuffer_);

        setCommonUniforms(*bufferShader_, width, height, time);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, noiseTexture_);
        bufferShader_->setInt("iChannel0", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, whiteTexture_);
        bufferShader_->setInt("iChannel1", 1);
        
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // Restore the default window framebuffer and prepare the final image.
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Pass 2: All effects are executed.
    // For standard effects, this is the only drawing pass; for Ndc3zl_image, this is the image pass. 
    setCommonUniforms(*shader_, width, height, time);

    
    if (selectedEffect_->interaction == InteractionMode::Moon) {
        shader_->setFloat("uZoom", moonInteraction_.zoom);
        shader_->setVec2("uViewCenter", moonInteraction_.viewCenter);
        shader_->setVec3("uMoon", moonInteraction_.moon);
        
        for (int i = 0; i < moonRippleCount; ++i) {
            shader_->setVec4("uRipples[" + std::to_string(i) + "]", moonInteraction_.ripples[i]);
        }
    }
    
    if (selectedEffect_->resources == ResourceProfile::GlyphAtlasAndNoise) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, matrixTextures_[0]);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, matrixTextures_[1]);
    }
    else if (selectedEffect_->resources == ResourceProfile::NoiseAndWhite) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, selectedEffect_->pipeline == Pipeline::TwoPass ? bufferColorTexture_ : noiseTexture_);
        shader_->setInt("iChannel0", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, whiteTexture_);
        shader_->setInt("iChannel1", 1);
    }

    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    if (glGetError() != GL_NO_ERROR) {
        SDL_Log("OpenGL error during shader (Matrix) rendering: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    SDL_GL_SwapWindow(window_);

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
    case SDL_EVENT_MOUSE_WHEEL:
        if (selectedEffect_->interaction == InteractionMode::Moon) {
            glm::vec2 anchor;
            if (cursorScenePosition(anchor)) {
                float wheelY = event->wheel.y;
                if (event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
                    wheelY = -wheelY;
                }
                auto& state = moonInteraction_;
                const float oldZoom = state.zoom;
                state.zoom = std::clamp(oldZoom * std::exp(wheelY * 0.12f), 0.65f, 3.0f);
                state.viewCenter = anchor - (anchor - state.viewCenter) * (oldZoom / state.zoom);
            }
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN: {
        if (selectedEffect_->interaction != InteractionMode::Moon) {
            break;
        }

        auto& state = moonInteraction_;

        if (event->button.button == SDL_BUTTON_RIGHT) {
            state.zoom = 1.0f;
            state.viewCenter = glm::vec2(0.0f);
            break;
        }

        if (event->button.button != SDL_BUTTON_LEFT) {
            break;
        }

        glm::vec2 point;
        if (!cursorScenePosition(point)) {
            break;
        }

        // Left-click the moon: Apply an elastic impulse.
        if (glm::length(point - glm::vec2(state.moon)) <= state.moon.z &&
            point.y > horizonHeight(point.x, state.frameTime)) {
            state.moonVelocity =
                std::min(state.moonVelocity + 5.0f, 6.0f);
            break;
        }

        // Left-click in other areas: sky is 1, water surface is 2.
        const float kind =
            point.y <= horizonHeight(point.x, state.frameTime)
                ? 2.0f
                : 1.0f;

        const float time =
            static_cast<float>(SDL_GetTicks() - startTicks_) / 1000.0f;

        state.ripples[state.nextRipple] =
            glm::vec4(point, time, kind);

        state.nextRipple =
            (state.nextRipple + 1) % moonRippleCount;
        break;
        }
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        glViewport(0, 0, event->window.data1, event->window.data2);
    }
    return SDL_APP_CONTINUE;
}

void App::quit(SDL_AppResult /*result*/) {
    // Clear OpenGL objects before destroy glContext
    if (glContext_) {
        if (vertexArray_ != 0) {
            glDeleteVertexArrays(1, &vertexArray_);
            vertexArray_ = 0;
        }

        glDeleteTextures(static_cast<GLsizei>(matrixTextures_.size()), matrixTextures_.data());
        matrixTextures_.fill(0);
        
        if (bufferShader_) {
            glDeleteProgram(bufferShader_->ID);
            bufferShader_.reset();
        }

        glDeleteTextures(1, &noiseTexture_);
        noiseTexture_ = 0;
        glDeleteTextures(1, &whiteTexture_);
        whiteTexture_ = 0;
        glDeleteTextures(1, &bufferColorTexture_);
        bufferColorTexture_ = 0;

        glDeleteFramebuffers(1, &bufferFramebuffer_);
        bufferFramebuffer_ = 0;
        bufferTargetWidth_ = 0;
        bufferTargetHeight_ = 0;

        if (shader_) {
            glDeleteProgram(shader_->ID);
            shader_.reset();
        }
        
        SDL_GL_DestroyContext(glContext_);
        glContext_ = nullptr;
    }
    
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    SDL_Quit();
}

void App::setCommonUniforms(const Shader &shader, int framebufferWidth,  int framebufferHeight, float time) {
    shader.use();

    shader.setVec2("uResolution", static_cast<float>(framebufferWidth), static_cast<float>(framebufferHeight));
    shader.setFloat("uTime", time);
    
    shader.setVec3("iResolution", static_cast<float>(framebufferWidth), static_cast<float>(framebufferHeight), 1.0f);
    shader.setFloat("iTime", time);
    
    int windowWidth = 0;
    int windowHeight = 0;
    SDL_GetWindowSize(window_, &windowWidth, &windowHeight);
    
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    SDL_GetMouseState(&mouseX, &mouseY);
    
    glm::vec2 mouse(0.0f);
    if (windowWidth > 0 && windowHeight > 0 && mouseX >= 0.0f && mouseY >= 0.0f && mouseX < windowWidth && mouseY < windowHeight) {
        // In SDL, mouse coordinates use the top-left corner of the window as the origin;
        // in GLSL, coordinates use the bottom-left corner as the origin and are based on the framebuffer's pixel dimensions.
        mouse.x = mouseX / windowWidth * framebufferWidth;
        mouse.y = (1.0f - mouseY / windowHeight) * framebufferHeight;
    }
    
    shader.setVec2("uMouse", mouse);
}

void App::updateMoon(float aspect, float time, float deltaTime) {
    constexpr float horizonBase = -0.12f;
    constexpr float damping = 4.5f;
    constexpr float frequency = 12.0f;
    
    auto& state = moonInteraction_;

    const float a = state.moonStretch;
    const float b = (state.moonVelocity + damping * a) / frequency;
    const float decay = std::exp(-damping * deltaTime);
    const float c = std::cos(frequency * deltaTime);
    const float s = std::sin(frequency * deltaTime);

    state.moonStretch = decay * (a * c + b * s);
    state.moonVelocity = decay * (
        (b * frequency - damping * a) * c -
        (a * frequency + damping * b) * s
    );

    state.moon = glm::vec3(
        aspect * 0.22f,
        horizonBase + moonRadius + 0.025f +
            0.23f * (1.0f - std::exp(-time * 0.018f)),
        moonRadius * (1.0f + state.moonStretch)
    );
    
    state.frameTime = time;
}

bool App::cursorScenePosition(glm::vec2 &point) const { 
    int windowWidth = 0;
    int windowHeight = 0;
    int framebufferWidth = 0;
    int framebufferHeight = 0;

    if (!SDL_GetWindowSize(window_, &windowWidth, &windowHeight) ||
        !SDL_GetWindowSizeInPixels(
            window_, &framebufferWidth, &framebufferHeight
        ) ||
        windowWidth <= 0 || windowHeight <= 0 ||
        framebufferWidth <= 0 || framebufferHeight <= 0) {
        return false;
    }

    float mouseX = 0.0f;
    float mouseY = 0.0f;
    SDL_GetMouseState(&mouseX, &mouseY);

    if (mouseX < 0.0f || mouseY < 0.0f ||
        mouseX >= windowWidth || mouseY >= windowHeight) {
        return false;
    }
    
    // SDL: Logical window coordinates, with the origin at the top-left.
    // Shader space: Origin at the bottom-left, with horizontal scaling adjusted based on the framebuffer's aspect ratio.
    const float aspect =
        static_cast<float>(framebufferWidth) / framebufferHeight;

    const glm::vec2 screen(
        (2.0f * mouseX / windowWidth - 1.0f) * aspect,
        1.0f - 2.0f * mouseY / windowHeight
    );

    point = moonInteraction_.viewCenter +
        screen / moonInteraction_.zoom;

    return true;
}

float App::horizonHeight(float x, float time) const { 
    constexpr float horizonBase = -0.12f;

    return horizonBase + 0.009f * std::sin(x * 4.2f - time * 0.42f) + 0.004f * std::sin(x * 10.7f + time * 0.63f + 1.4f) + 0.002f * std::sin(x * 22.0f - time * 0.85f + 0.6f);
}

bool App::ensureBufferTarget(int width, int height) { 
    if (bufferFramebuffer_ != 0 && bufferColorTexture_ != 0 && bufferTargetWidth_ == width && bufferTargetHeight_ == height) {
        return true;
    }

    glDeleteFramebuffers(1, &bufferFramebuffer_);
    glDeleteTextures(1, &bufferColorTexture_);
    
    bufferFramebuffer_ = 0;
    bufferColorTexture_ = 0;
    bufferTargetWidth_ = 0;
    bufferTargetHeight_ = 0;
    
    glActiveTexture(GL_TEXTURE0);

    if (!shader_test::createBufferTarget(width, height, bufferFramebuffer_, bufferColorTexture_)) {
        SDL_Log("up_in_cloud_sea Buffer frame buffer is incomplete: %s", SDL_GetError());
        return false;
    }
    
    bufferTargetWidth_ = width;
    bufferTargetHeight_ = height;
    return true; 
}