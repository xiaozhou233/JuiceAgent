#pragma once

#include <JuiceAgent/Logger.hpp>
#include <tinytoml/toml.h>
#include <filesystem>
#include <fstream>

namespace config
{
    class Config {
    private:
        toml::Value config{};
        bool valid = false;

    public:
        explicit Config(const char* c_runtime_dir) {
            std::filesystem::path path = std::filesystem::path(c_runtime_dir) / "config.toml";

            std::ifstream ifs(path);
            if (!ifs.is_open()) {
                PLOGE << "Failed to open config file: " << path;
                return;
            }

            try {
                auto pr = toml::parse(ifs);

                if (!pr.valid()) {
                    PLOGE << "Invalid TOML file: " << pr.errorReason;
                    return;
                }

                config = pr.value;
                valid = true;
            } catch (const std::exception& e) {
                PLOGE << "TOML parse error: " << e.what();
            }
        }

        bool is_valid() const {
            return valid;
        }
    };
}