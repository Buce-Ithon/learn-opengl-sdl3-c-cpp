#include "platform/asset_locator.h"

#include "SDL3/SDL_error.h"
#include "SDL3/SDL_filesystem.h"

#include <stdexcept>

std::filesystem::path shaderDirectory() {
    const char* basePath = SDL_GetBasePath();
    if (basePath == nullptr) throw std::runtime_error(std::string("Unable to determine application directory: ") + SDL_GetError());
    return std::filesystem::path(basePath) / "assets" / "shaders";
}
