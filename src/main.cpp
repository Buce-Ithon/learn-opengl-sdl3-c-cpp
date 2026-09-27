#include <iostream>
#include "SDL3/SDL_Log.h"

int main() {
    std::cout << "Hello, SDL3!" << std::endl;
    SDL_Log("%s", "Hello, SDL3! from SDL_Log~");
    return 0;
}