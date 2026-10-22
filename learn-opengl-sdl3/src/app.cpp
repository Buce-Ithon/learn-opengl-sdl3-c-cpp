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

#include <string>

SDL_AppResult App::init(int /*argc*/, char** /*argv*/) {
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
    shader_ = std::make_unique<Shader>("./assets/shaders/fullscreen.vert", "./assets/shaders/mt_fuji_watercolor.frag");
    GLint linked = GL_FALSE;
    glGetProgramiv(shader_->ID, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        SDL_Log("Failed to link the Matrix shader program: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    matrixTextures_ = matrix::createTextures();
    shader_->use();
    shader_->setInt("iChannel0", 0);
    shader_->setInt("iChannel1", 1);
    for (int i = 0; i < 2; ++i) {
        shader_->setVec3("iChannelResolution[" + std::to_string(i) + "]", static_cast<float>(matrix::textureSize), static_cast<float>(matrix::textureSize), 1.0f);
    }
    glGenVertexArrays(1, &vertexArray_);
    startTicks_ = SDL_GetTicks();
    
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
    
    const float time = static_cast<float>(SDL_GetTicks() - startTicks_) / 1000.0f;
    
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    shader_->use();
    setCommonUniforms(*shader_, width, height, time);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, matrixTextures_[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, matrixTextures_[1]);
    glBindVertexArray(vertexArray_);
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
        
        if (shader_) {
            glDeleteProgram(shader_->ID);
            shader_.reset();
        }
        
        SDL_GL_DestroyContext(glContext_);
        glContext_ = nullptr;
    }
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