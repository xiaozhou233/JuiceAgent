#pragma once

#include <filesystem>
#include <optional>
#include <toml++/toml.h>
#include <string>
#include <vector>
#include <cstddef>
#include <JuiceAgent/Logger.hpp>

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

        // Load the config file.
        //
        // This must never throw: when libloader runs from the reflectively
        // loaded DLL its unwind tables are not registered, so a C++ exception
        // raised here could not be caught and would crash the target process.
        // toml++ switches to non-throwing mode (parse_result) automatically
        // when exceptions are disabled.
#if TOML_EXCEPTIONS
        try {
            _config = toml::parse_file(_config_path.string());
            _valid = true;
        } catch (const toml::parse_error& e) {
            spdlog::error("Failed to parse config file: {}", e.description());
            _last_error = e;
        }
#else
        toml::parse_result result = toml::parse_file(_config_path.string());
        if (result.succeeded()) {
            _config = std::move(result).table();
            _valid = true;
        } else {
            spdlog::error("Failed to parse config file: {}", result.error().description());
            _last_error = result.error();
        }
#endif
    }

    // Non-throwing lookup: returns defaultValue when the key is missing or has
    // the wrong type. There is deliberately no throwing overload - callers that
    // require a key must check contains() first.
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

} // namespace JuiceAgent::Config
