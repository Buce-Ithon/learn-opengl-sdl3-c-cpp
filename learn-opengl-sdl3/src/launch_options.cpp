#include "app/launch_options.h"

#include <stdexcept>
#include <string_view>

LaunchOptions parseLaunchOptions(int argc, char** argv) {
    LaunchOptions options{&availableEffects().front()};
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument = argv[i];
        if (argument == "--list") { options.listEffects = true; continue; }
        if (argument == "--effect") {
            if (++i == argc) throw std::invalid_argument("--effect requires a name.");
            options.effect = findEffect(argv[i]);
            if (options.effect == nullptr) throw std::invalid_argument(std::string("Unknown effect: ") + argv[i]);
            continue;
        }
        throw std::invalid_argument(std::string("Unknown argument: ") + argv[i]);
    }
    return options;
}
