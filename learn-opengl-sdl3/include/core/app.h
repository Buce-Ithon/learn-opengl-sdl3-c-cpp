#pragma once

#include "SDL3/SDL_init.h"
#include <array>
#include <memory>
#include "shader.h"
#include "glm/glm.hpp"

enum class Pipeline {
    Single, // Single-pass shader that does not rely on additional textures or off-screen rendering
    Textured // Requires creating and binding an additional texture, as well as setting a sampler uniform.
};

enum class InteractionMode {
    None,
    Moon
};

struct Effect {
    const char* name;
    const char* fragment;
    Pipeline pipeline;
    const char* description;
    InteractionMode interaction;
};

inline constexpr int moonRippleCount = 8;
inline constexpr float moonRadius = 0.155f;

struct MoonInteraction {
    float zoom = 1.0f;
    glm::vec2 viewCenter{0.0f};

    // xy: Moon center; z: Radius(influenced by stretching)
    glm::vec3 moon{0.0f, 0.0f, moonRadius};
    float moonStretch = 0.0f;
    float moonVelocity = 0.0f;
    float frameTime = 0.0f;
    
    // xy: scene coordinates; z: start time; w: 0 = inactive, 1 = sky, 2 = water surface.
    std::array<glm::vec4, moonRippleCount> ripples{};
    int nextRipple = 0;
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
    
    MoonInteraction moonInteraction_;
    Uint64 previousFrameTicks_ = 0;
    void updateMoon(float aspect, float time, float deltaTime);
};