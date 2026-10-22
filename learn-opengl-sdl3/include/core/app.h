#pragma once

#include "SDL3/SDL_init.h"
#include <array>
#include <memory>
#include "shader.h"

enum class Pipeline {
    Single,
    Matrix
};

struct Effect {
    const char* name;
    const char* fragment;
    Pipeline pipeline;
    const char* description;
};

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
    std::array<GLuint, 2> matrixTextures_{};
    GLuint vertexArray_ = 0;
    Uint64 startTicks_ = 0;
    
    void setCommonUniforms(const Shader& shader, int framebufferWidth, int framebufferHeight, float time);
    
    const Effect* selectedEffect_ = nullptr;
};