#pragma once

#include <filesystem>
#include <string>
#include <type_traits>

#include <JuiceAgent/JuiceAgent.hpp>
#include <JuiceAgent/Logger.hpp>
#include <JuiceAgent/Utils.hpp>
#include <toml.hpp>

namespace JuiceAgent::Config {

class Config {
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
        }
        else {
            spdlog::error("Failed to parse config.toml");

            for (const auto& err : result.unwrap_err()) {
                spdlog::error("{}", toml::format_error(err));
            }
        }
    }

    bool is_valid() const noexcept
    {
        return _valid;
    }

    const toml::value& get_config() const noexcept
    {
        return _config;
    }

    const std::filesystem::path& runtime_dir() const noexcept
    {
        return _runtime_dir;
    }

    template<typename T>
    T get(const std::string& path, const T& default_value, bool is_path = false) const
    {
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

            T value = read_value<T>(*current);

            if constexpr (std::is_same_v<T, std::string>) {
                if (value.empty()) {
                    value = default_value;
                }

                if (is_path && value.starts_with(".")) {
                    value = (_runtime_dir / value).lexically_normal().string();
                }
            }

            return value;
        }
        catch (const std::exception& e) {
            spdlog::warn("Config get failed: {}, using default value", path);
            return default_value;
        }
    }

private:
    template<typename T>
    static T read_value(const toml::value& v)
    {
        if constexpr (std::is_same_v<T, std::string>) {
            return v.as_string();
        }
        else if constexpr (std::is_same_v<T, bool>) {
            return v.as_boolean();
        }
        else if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
            return static_cast<T>(v.as_integer());
        }
        else if constexpr (std::is_floating_point_v<T>) {
            return static_cast<T>(v.as_floating());
        }
        else {
            static_assert(sizeof(T) == 0, "Unsupported config type");
        }
    }

    std::filesystem::path _runtime_dir;
    toml::value _config;
    bool _valid = false;
};

namespace Utils {
    inline LoaderConfig get_loader_config(const Config& config)
    {
        LoaderConfig loader_config;
        loader_config.JuiceAgentNativeLibraryPath = config.get<std::string>(
            "JuiceAgent.Loader.JuiceAgentNativeLibraryPath",
            (config.runtime_dir() / "libagent.dll").string(),
            true
        );
        loader_config.RuntimeDir = config.runtime_dir().string();

        return loader_config;
    }

    inline void print_loader_config(const LoaderConfig& config)
    {
        spdlog::info("JuiceAgentNativeLibraryPath: {}", config.JuiceAgentNativeLibraryPath);
        spdlog::info("RuntimeDir: {}", config.RuntimeDir);
    }

    inline std::string serialize_loader_config(const LoaderConfig& config)
    {
        JuiceAgent::Utils::Serializer ser;
        ser.add_kv("Version", 2);
        ser.add_kv("JuiceAgentNativeLibraryPath", config.JuiceAgentNativeLibraryPath);
        ser.add_kv("RuntimeDir", config.RuntimeDir);

        return ser.serialize();
    }
}

} // namespace JuiceAgent::Config
