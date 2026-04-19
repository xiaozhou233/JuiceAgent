#pragma once

#include <JuiceAgent/Logger.hpp>
#include <JuiceAgent/JuiceAgent.hpp>
#include <toml.hpp>
#include <filesystem>
#include <string>

namespace JuiceAgent::Config {

class Config {
private:
    std::filesystem::path _runtime_dir;
    toml::value _config;
    bool _valid = false;

public:
    explicit Config(const std::filesystem::path& runtime_dir = {})
    {
        _runtime_dir = runtime_dir.empty()
            ? std::filesystem::current_path()
            : runtime_dir;

        const auto result = toml::try_parse(_runtime_dir / "config.toml");

        if (result.is_ok()) {
            _config = result.unwrap();
            _valid = true;
        } else {
            PLOGE << "Failed to parse config.toml";

            for (const auto& err : result.unwrap_err()) {
                PLOGE << toml::format_error(err);
            }
        }
    }

    bool is_valid() const noexcept {
        return _valid;
    }

    const toml::value& get_config() const noexcept {
        return _config;
    }

    const std::filesystem::path& runtime_dir() const noexcept {
        return _runtime_dir;
    }
};

namespace Utils {
    inline LoaderConfig get_loader_config(const Config& config) {
        auto cfg = config.get_config().at("JuiceAgent").at("Loader");
        
        LoaderConfig loader_config = LoaderConfig();
        loader_config.JuiceAgentAPIJarPath = cfg.at("JuiceAgentAPIJarPath").as_string();
        loader_config.JuiceAgentNativeLibraryPath = cfg.at("JuiceAgentNativeLibraryPath").as_string();
        loader_config.RuntimeDir = config.runtime_dir().string();

        return loader_config;
    }

    inline void print_loader_config(const LoaderConfig& config) {
        PLOGI << "JuiceAgentAPIJarPath: " << config.JuiceAgentAPIJarPath;
        PLOGI << "JuiceAgentNativeLibraryPath: " << config.JuiceAgentNativeLibraryPath;
        PLOGI << "RuntimeDir: " << config.RuntimeDir;
    }

    inline std::string serialize_loader_config(const LoaderConfig& config) {
        std::string result = "";

        result += "Version=2;";
        result += "JuiceAgentAPIJarPath=" + config.JuiceAgentAPIJarPath + ";";
        result += "JuiceAgentNativeLibraryPath=" + config.JuiceAgentNativeLibraryPath + ";";
        result += "RuntimeDir=" + config.RuntimeDir + ";";
        
        return result;
    }
}

} // namespace JuiceAgent::Config