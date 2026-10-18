#include "JuiceAgent/Logger.hpp"
#include "Loader.hpp"

#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>
#include <thread>

namespace fs = std::filesystem;

static void run_loader() {
    const char* env = std::getenv("JuiceAgent_ConfigPath");
    std::string current_path;

    if (env != nullptr && env[0] != '\0') {
        // Use the environment variable if it exists and is not empty.
        current_path = env;
    } else {
        spdlog::trace("[libloader] JuiceAgent_ConfigPath is not set");

        const char* home = std::getenv("HOME");
        if (home == nullptr || home[0] == '\0') {
            spdlog::error("[libloader] HOME environment variable is not set");
            return;
        }

        // Fall back to $HOME/.juiceagent.
        fs::path config_dir = fs::path(home) / ".juiceagent";

        std::error_code ec;
        fs::create_directories(config_dir, ec);

        if (ec) {
            spdlog::error(
                "[libloader] Failed to create config directory: {}",
                ec.message()
            );
            return;
        }

        current_path = config_dir.string();
    }

    spdlog::info("[libloader] current path: {}", current_path);

    JuiceAgent::Loader::entrypoint(current_path.c_str());
}

__attribute__((constructor))
static void entry() {
    Logger::init();

    spdlog::info("[libloader] loaded!");

    std::thread([] {
        run_loader();
    }).detach();
}