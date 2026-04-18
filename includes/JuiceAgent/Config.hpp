#pragma once

#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

#include <JuiceAgent/Logger.hpp>
#include <toml.hpp>

namespace JuiceAgent::Config {

class Config {

private:
    std::filesystem::path config_dir_;
    toml::value data_;
    bool valid_ = false;

private:
    // Split "A.B.C" -> {"A", "B", "C"}
    static std::vector<std::string> split_path(const std::string& path) {
        std::vector<std::string> result;
        std::stringstream stream(path);
        std::string token;

        while (std::getline(stream, token, '.')) {
            if (!token.empty()) {
                result.push_back(token);
            }
        }

        return result;
    }

    // Find node by dot path
    const toml::value* find(const std::string& path) const {
        const toml::value* current = &data_;

        for (const auto& key : split_path(path)) {
            if (!current->is_table()) {
                return nullptr;
            }

            if (!current->contains(key)) {
                return nullptr;
            }

            current = &current->at(key);
        }

        return current;
    }

    // Load and parse config file
    void load() {
        if (!std::filesystem::exists(config_dir_)) {
            PLOGE << "Config directory not found: " << config_dir_;
            config_dir_ = std::filesystem::current_path();
            PLOGW << "Using runtime directory: " << std::filesystem::current_path();
        }

        const auto path = config_dir_ / "config.toml";

        if (!std::filesystem::exists(path)) {
            PLOGE << "Config file not found: " << path;
            return;
        }

        try {
            data_ = toml::parse(path);
            valid_ = true;
        } catch (const std::exception& e) {
            PLOGE << "Failed to parse config: " << e.what();
        }
    }

public:
    explicit Config(std::filesystem::path dir)
        : config_dir_(std::move(dir)) {
        load();
    }

public:
    bool is_valid() const {
        return valid_;
    }

    bool contains(const std::string& path) const {
        return find(path) != nullptr;
    }

    template<typename T>
    T get(const std::string& path) const {
        const auto* node = find(path);

        if (!node) {
            throw std::runtime_error("Config key not found: " + path);
        }

        return toml::get<T>(*node);
    }

    template<typename T>
    T get_or(const std::string& path, const T& default_value) const {
        const auto* node = find(path);

        if (!node) {
            return default_value;
        }

        try {
            auto result = toml::get<T>(*node);
            if (result.empty())
                return default_value;
            return result;
        } catch (...) {
            return default_value;
        }
    }

    std::string get_path_or_default(const std::string& path, const std::string& default_value) const {
        std::string result = get_or<std::string>(path, default_value);
        if(result.starts_with(".")) {
            result.erase(0, 1);

            // Check if string starts with '/'
            // MUST REMOVE '/', or it will cause path error (root directory)
            if (result.starts_with('/')) {
                result.erase(0, 1);
            }

            result = (config_dir_ / result).string();
        }

        return result;
    }

    InjectionInfo get_injection_info() const {
        InjectionInfo info;

        auto get_default_path = [&](const std::string& name) {
            return (std::filesystem::path(config_dir_) / name).string();
        };

        info.JuiceAgentAPIJarPath = get_path_or_default("Loader.JuiceAgentAPIJarPath", get_default_path("JuiceAgentAPI.jar"));
        info.JuiceAgentNativeLibraryPath = get_path_or_default("Loader.JuiceAgentNativeLibraryPath", get_default_path("libagent.dll"));
        info.ConfigDir = config_dir_.string();

        return info;
    }

    static std::string serialize(InjectionInfo info) {

        auto escape = [](const std::string& value) -> std::string {
            std::string result;
            result.reserve(value.size());

            for (char ch : value) {
                if (ch == ';' || ch == '=' || ch == '\\') {
                    result += '\\';
                }
                result += ch;
            }

            return result;
        };

        return
            "JuiceAgentAPIJarPath=" + escape(info.JuiceAgentAPIJarPath) + ";" +
            "JuiceAgentNativeLibraryPath=" + escape(info.JuiceAgentNativeLibraryPath) + ";" +
            "ConfigDir=" + escape(info.ConfigDir);
    }
};

void print_injection_info(const InjectionInfo& info) {
    
    PLOGI << "JuiceAgentAPIJarPath: " << info.JuiceAgentAPIJarPath;
    PLOGI << "JuiceAgentNativeLibraryPath: " << info.JuiceAgentNativeLibraryPath;
    PLOGI << "ConfigDir: " << info.ConfigDir;
}


} // namespace JuiceAgent::Config