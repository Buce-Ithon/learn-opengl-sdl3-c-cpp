#pragma once

#include "effects/effect.h"

struct LaunchOptions {
    const Effect* effect = nullptr;
    bool listEffects = false;
};

[[nodiscard]] LaunchOptions parseLaunchOptions(int argc, char** argv);
