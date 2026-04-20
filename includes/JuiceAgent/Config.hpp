#pragma once

#include <JuiceAgent/Logger.hpp>
#include <JuiceAgent/JuiceAgent.hpp>
#include <toml.hpp>
#include <filesystem>
#include <string>
#include <JuiceAgent/Utils.hpp>

namespace JuiceAgent::Config {

class Config {
private:
    std::filesystem::path _runtime_dir;
    toml::value _config;
    bool _valid = false;

private:
    template<typename T>
    static T _read_value(const toml::value& v) {
        if constexpr (std::is_same_v<T, std::string>) {
            return v.as_string();
        } else if constexpr (std::is_same_v<T, bool>) {
            return v.as_boolean();
        } else if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
            return static_cast<T>(v.as_integer());
        } else if constexpr (std::is_floating_point_v<T>) {
            return static_cast<T>(v.as_floating());
        } else {
            static_assert(sizeof(T) == 0, "Unsupported config type");
        }
    }

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

    template<typename T>
    T get(const std::string& path, const T& default_value, bool is_path = false) const {
        if (!_valid) {
            return default_value;
        }

        try {
            const toml::value* current = &_config;
            std::size_t start = 0;

            while (start < path.size()) {
                const std::size_t end = path.find('.', start);
                const std::string key = path.substr(
                    start,
                    end == std::string::npos ? std::string::npos : end - start
                );

                current = &current->at(key);

                if (end == std::string::npos) {
                    break;
                }

                start = end + 1;
            }

            T value = _read_value<T>(*current);

            if constexpr (std::is_same_v<T, std::string>) {
                if (value.empty()) {
                    return default_value;
                }

                // Handle relative paths
                if (is_path) {
                    // Check if starts with "."
                    if (value.starts_with(".")) {
                        // Remove "."
                        value = value.substr(1);
                        // Check if starts with "/"
                        if (value.starts_with("/")) {
                            // Remove "/"
                            value = value.substr(1);
                        }

                        // runtime_dir + value
                        value = (_runtime_dir / value).string();
                    }
                }

            }

            return value;
        }
        catch (const std::exception& e) {
            PLOGW << "Config get failed: " << path << ", using default value";
            return default_value;
        }
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
        // std::string result = "";

        // result += "Version=2;";
        // result += "JuiceAgentAPIJarPath=" + config.JuiceAgentAPIJarPath + ";";
        // result += "JuiceAgentNativeLibraryPath=" + config.JuiceAgentNativeLibraryPath + ";";
        // result += "RuntimeDir=" + config.RuntimeDir + ";";

        JuiceAgent::Utils::Serializer ser;
        ser.add_kv("Version", 2);
        ser.add_kv("JuiceAgentAPIJarPath", config.JuiceAgentAPIJarPath);
        ser.add_kv("JuiceAgentNativeLibraryPath", config.JuiceAgentNativeLibraryPath);
        ser.add_kv("RuntimeDir", config.RuntimeDir);

        return ser.serialize();
    }
}

} // namespace JuiceAgent::Config