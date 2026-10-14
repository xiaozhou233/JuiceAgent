#pragma once

#include <filesystem>
#include <optional>
#include <toml++/toml.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstddef>
#include <JuiceAgent/Logger.hpp>
#include <JuiceAgent/Utils.hpp>
#include <JuiceAgent/ConfigStruct.hpp>

namespace JuiceAgent::Config {

class Config {
private:
    toml::table _config;
    std::optional<toml::parse_error> _last_error;

    std::filesystem::path _runtime_dir;
    std::filesystem::path _config_path;

    bool _valid = false;

public:
    explicit Config(const std::filesystem::path& runtime_dir = {}) {
        // Check if the runtime directory is empty
        if (runtime_dir.empty()) {
            // Use the default runtime directory
            _runtime_dir = std::filesystem::current_path();
        } else {
            _runtime_dir = runtime_dir;
        }

        _config_path = _runtime_dir / "config.toml";

        // Load the config file
        try {
            _config = toml::parse_file(_config_path.string());
            _valid = true;
        } catch (const toml::parse_error& e) {
            spdlog::error("Failed to parse config file: {}", e.what());
            _last_error = e;
        }
    }


    template<typename T>
    T get(const std::string& path) const {
        const auto* node = _config.at_path(path).node();

        if (!node) {
            spdlog::error("Configuration key not found: {}", path);
            throw std::runtime_error(
                "Configuration key not found: " + path
            );
        }

        auto value = node->value<T>();

        if (!value) {
            spdlog::error("Configuration type mismatch: {}", path);
            throw std::runtime_error(
                "Configuration type mismatch: " + path
            );
        }

        return *value;
    }

    template<typename T>
    T get(const std::string& path, T defaultValue) const {
        const auto* node = _config.at_path(path).node();

        if (!node) {
            // Configuration key is missing; use the default value.
            spdlog::warn("Configuration key not found: {}, using default value", path);
            return defaultValue;
        }

        auto value = node->value<T>();

        if (!value) {
            // Configuration type mismatch; use the default value.
            spdlog::warn("Configuration type mismatch: {}, using default value", path);
            return defaultValue;
        }

        return *value;
    }

    std::string getPath(const std::string& key) const {
        const std::string value = get<std::string>(key);

        if (value.empty()) {
            return {};
        }

        return getPath(key, value);
    }

    std::string getPath(
        const std::string& key,
        const std::string& defaultValue
    ) const {
        std::string rawValue = get<std::string>(key, defaultValue);

        // Use the default value when the configuration value is empty.
        if (rawValue.empty()) {
            rawValue = defaultValue;
        }

        if (rawValue.empty()) {
            return {};
        }

        const std::filesystem::path value(rawValue);

        if (value.is_absolute()) {
            return value.lexically_normal().string();
        }

        const std::string normalized = value.generic_string();

        if (normalized == "." || normalized.starts_with("./")) {
            return (_runtime_dir / value).lexically_normal().string();
        }

        return value.lexically_normal().string();
    }

    template<typename T>
    std::vector<T> getArray(const std::string& path) const {
        const auto* node = _config.at_path(path).node();

        if (!node) {
            throw std::runtime_error(
                "Configuration key not found: " + path
            );
        }

        const auto* array = node->as_array();

        if (!array) {
            throw std::runtime_error(
                "Configuration type mismatch: expected array: " + path
            );
        }

        std::vector<T> result;
        result.reserve(array->size());

        for (std::size_t i = 0; i < array->size(); ++i) {
            const auto& element = (*array)[i];
            auto value = element.value<T>();

            if (!value) {
                throw std::runtime_error(
                    "Configuration array element type mismatch: "
                    + path + "[" + std::to_string(i) + "]"
                );
            }

            result.push_back(*value);
        }

        return result;
    }

    template<typename T>
    std::vector<T> getArray(
        const std::string& path,
        std::vector<T> defaultValue
    ) const {
        const auto* node = _config.at_path(path).node();

        if (!node) {
            return defaultValue;
        }

        const auto* array = node->as_array();

        if (!array) {
            throw std::runtime_error(
                "Configuration type mismatch: expected array: " + path
            );
        }

        std::vector<T> result;
        result.reserve(array->size());

        for (std::size_t i = 0; i < array->size(); ++i) {
            const auto& element = (*array)[i];
            auto value = element.value<T>();

            if (!value) {
                throw std::runtime_error(
                    "Configuration array element type mismatch: "
                    + path + "[" + std::to_string(i) + "]"
                );
            }

            result.push_back(*value);
        }

        return result;
    }

    bool contains(const std::string& path) const {
        return _config.at_path(path).node() != nullptr;
    }

    // Getters
    const toml::table& getConfig() const {
        return _config;
    }

    const std::filesystem::path& getRuntimeDir() const {
        return _runtime_dir;
    }

    const std::filesystem::path& getConfigPath() const {
        return _config_path;
    }

    const std::optional<toml::parse_error>& getLastError() const {
        return _last_error;
    }

    bool isValid() const {
        return _valid;
    }
};

namespace Utils {
    inline LoaderConfig get_loader_config(const Config& config)
    {
        LoaderConfig loader_config;
        loader_config.JuiceAgentNativeLibraryPath = config.getPath(
            "JuiceAgent.Loader.JuiceAgentNativeLibraryPath",
            (config.getRuntimeDir() / "libagent.dll").string()
        );
        loader_config.RuntimeDir = config.getRuntimeDir().string();

        return loader_config;
    }

    inline void print_loader_config(const LoaderConfig& config)
    {
        spdlog::info("JuiceAgentNativeLibraryPath: {}", config.JuiceAgentNativeLibraryPath);
        spdlog::info("RuntimeDir: {}", config.RuntimeDir);
    }

    inline std::string serialize_loader_config(const Config& config, const LoaderConfig& loader_config)
    {
        JuiceAgent::Utils::Serializer ser;
        ser.add_kv("Version", config.get<int>("JuiceAgent.Version", -1));
        ser.add_kv("JuiceAgentNativeLibraryPath", loader_config.JuiceAgentNativeLibraryPath);
        ser.add_kv("RuntimeDir", loader_config.RuntimeDir);

        return ser.serialize();
    }
}

} // namespace JuiceAgent::Config