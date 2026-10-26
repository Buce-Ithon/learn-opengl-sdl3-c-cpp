#pragma once

#include "SDL3/SDL_init.h"
#include <array>
#include <memory>
#include "shader.h"
#include "gl_objects.h"
#include "effects/effect.h"
#include "effects/moon_controller.h"

class App {
public:
    App()  = default;
    ~App() = default;
    
    // 4 trampoline invokes
    SDL_AppResult init(int argc, char** argv);
    SDL_AppResult iterate();
    SDL_AppResult event(SDL_Event* event);
    void          quit(SDL_AppResult result);
    
private:
    SDL_Window*   window_    = nullptr;
    SDL_GLContext glContext_ = nullptr;
    //bool          running_   = true;
    std::unique_ptr<Shader> shader_;
    std::array<Texture2D, 2> matrixTextures_{};
    VertexArray vertexArray_;
    Uint64 startTicks_ = 0;
    
    void setCommonUniforms(const Shader& shader, int framebufferWidth, int framebufferHeight, float time);
    
    const Effect* selectedEffect_ = nullptr;
    
    MoonController moonController_;
    Uint64 previousFrameTicks_ = 0;
    bool cursorScenePosition(glm::vec2& point) const;
    
    std::unique_ptr<Shader> bufferShader_;
    Texture2D noiseTexture_;
    Texture2D whiteTexture_;
    Framebuffer bufferFramebuffer_;
    Texture2D bufferColorTexture_;
    int bufferTargetWidth_  = 0;
    int bufferTargetHeight_ = 0;
    bool ensureBufferTarget(int width, int height);
};
