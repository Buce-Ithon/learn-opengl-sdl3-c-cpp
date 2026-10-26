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
#include "SDL3/SDL_filesystem.h"
#include "misc/shader_test_resources.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <cmath>
#include <algorithm>

namespace {
std::filesystem::path shaderPath(std::string_view filename) {
    const char* basePath = SDL_GetBasePath();
    if (basePath == nullptr) {
        throw std::runtime_error(std::string("Unable to determine application directory: ") + SDL_GetError());
    }
    return std::filesystem::path(basePath) / "assets" / "shaders" / filename;
}
}

SDL_AppResult App::init(int argc, char** argv) {
    // Parse command-line arguments
    selectedEffect_ = &availableEffects().front();

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--list") {
            for (const auto& effect : availableEffects()) {
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
    std::filesystem::path vertexPath;
    try {
        vertexPath = shaderPath("fullscreen.vert");
        const std::filesystem::path fragmentPath = shaderPath(selectedEffect_->fragment);
        shader_ = std::make_unique<Shader>(vertexPath.string().c_str(), fragmentPath.string().c_str());
    } catch (const std::exception& error) {
        SDL_Log("Failed to create shader program: %s", error.what());
        return SDL_APP_FAILURE;
    }
    if (selectedEffect_->resources == ResourceProfile::GlyphAtlasAndNoise) {
        const auto textureIds = matrix::createTextures();
        for (std::size_t i = 0; i < matrixTextures_.size(); ++i) matrixTextures_[i].reset(textureIds[i]);

        shader_->use();
        shader_->setInt("iChannel0", 0);
        shader_->setInt("iChannel1", 1);

        for (int i = 0; i < 2; ++i) {
            shader_->setVec3("iChannelResolution[" + std::to_string(i) + "]", static_cast<float>(matrix::textureSize), static_cast<float>(matrix::textureSize), 1.0f);
        }
    }
    if (selectedEffect_->resources == ResourceProfile::NoiseAndWhite) {
        noiseTexture_.reset(shader_test::createNoiseTexture());
        whiteTexture_.reset(shader_test::createWhiteTexture());
    }
    if (selectedEffect_->pipeline == Pipeline::TwoPass) {
        try {
            const std::filesystem::path bufferPath = shaderPath("up_in_cloud_sea_buffer.frag");
            bufferShader_ = std::make_unique<Shader>(vertexPath.string().c_str(), bufferPath.string().c_str());
        } catch (const std::exception& error) {
            SDL_Log("Failed to create buffer shader program: %s", error.what());
            return SDL_APP_FAILURE;
        }
    }
    vertexArray_.create();
    startTicks_ = SDL_GetTicks();
    previousFrameTicks_ = startTicks_;
    
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
        moonController_.update(static_cast<float>(width) / static_cast<float>(height), time, deltaTime);
    }
    
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glBindVertexArray(vertexArray_.id());

    // Pass 1: Only up_in_cloud_sea_image need it.
    // Buffer shader render to an off-screen color texture, rather than directly to the window.
    if (selectedEffect_->pipeline == Pipeline::TwoPass) {
        if (!ensureBufferTarget(width, height)) {
            return SDL_APP_FAILURE;
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, bufferFramebuffer_.id());

        setCommonUniforms(*bufferShader_, width, height, time);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, noiseTexture_.id());
        bufferShader_->setInt("iChannel0", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, whiteTexture_.id());
        bufferShader_->setInt("iChannel1", 1);
        
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // Restore the default window framebuffer and prepare the final image.
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Pass 2: All effects are executed.
    // For standard effects, this is the only drawing pass; for Ndc3zl_image, this is the image pass. 
    setCommonUniforms(*shader_, width, height, time);

    
    if (selectedEffect_->interaction == InteractionMode::Moon) {
        const MoonInteraction& moon = moonController_.state();
        shader_->setFloat("uZoom", moon.zoom);
        shader_->setVec2("uViewCenter", moon.viewCenter);
        shader_->setVec3("uMoon", moon.moon);
        
        for (int i = 0; i < moonRippleCount; ++i) {
            shader_->setVec4("uRipples[" + std::to_string(i) + "]", moon.ripples[i]);
        }
    }
    
    if (selectedEffect_->resources == ResourceProfile::GlyphAtlasAndNoise) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, matrixTextures_[0].id());

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, matrixTextures_[1].id());
    }
    else if (selectedEffect_->resources == ResourceProfile::NoiseAndWhite) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, selectedEffect_->pipeline == Pipeline::TwoPass ? bufferColorTexture_.id() : noiseTexture_.id());
        shader_->setInt("iChannel0", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, whiteTexture_.id());
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
                moonController_.zoomAt(anchor, wheelY);
            }
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN: {
        if (selectedEffect_->interaction != InteractionMode::Moon) {
            break;
        }

        if (event->button.button == SDL_BUTTON_RIGHT) {
            moonController_.resetView();
            break;
        }

        if (event->button.button != SDL_BUTTON_LEFT) {
            break;
        }

        glm::vec2 point;
        if (!cursorScenePosition(point)) {
            break;
        }

        const float time =
            static_cast<float>(SDL_GetTicks() - startTicks_) / 1000.0f;
        moonController_.click(point, time);
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
        vertexArray_.reset();
        for (auto& texture : matrixTextures_) texture.reset();
        
        bufferShader_.reset();

        noiseTexture_.reset();
        whiteTexture_.reset();
        bufferColorTexture_.reset();
        bufferFramebuffer_.reset();
        bufferTargetWidth_ = 0;
        bufferTargetHeight_ = 0;

        shader_.reset();
        
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
    
    point = moonController_.scenePosition(
        {mouseX, mouseY},
        {static_cast<float>(windowWidth), static_cast<float>(windowHeight)},
        {static_cast<float>(framebufferWidth), static_cast<float>(framebufferHeight)}
    );

    return true;
}

bool App::ensureBufferTarget(int width, int height) { 
    if (bufferFramebuffer_.id() != 0 && bufferColorTexture_.id() != 0 && bufferTargetWidth_ == width && bufferTargetHeight_ == height) {
        return true;
    }

    bufferFramebuffer_.reset();
    bufferColorTexture_.reset();
    bufferTargetWidth_ = 0;
    bufferTargetHeight_ = 0;
    
    glActiveTexture(GL_TEXTURE0);

    GLuint framebuffer = 0;
    GLuint colorTexture = 0;
    if (!shader_test::createBufferTarget(width, height, framebuffer, colorTexture)) {
        SDL_Log("up_in_cloud_sea Buffer frame buffer is incomplete: %s", SDL_GetError());
        return false;
    }
    bufferFramebuffer_.reset(framebuffer);
    bufferColorTexture_.reset(colorTexture);
    
    bufferTargetWidth_ = width;
    bufferTargetHeight_ = height;
    return true; 
}
