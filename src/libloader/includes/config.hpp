#pragma once

#include <JuiceAgent/Logger.hpp>
#include <JuiceAgent/JuiceAgent.hpp>
#include <tinytoml/toml.h>
#include <filesystem>
#include <fstream>
#include <string>

namespace config {

class Config {
private:
    std::string runtime_dir; // Runtime directory
    toml::Value config{};    // Parsed TOML configuration
    bool valid = false;      // Whether the config is successfully loaded

    // Escape special characters ';' = '\'
    static std::string escape(const std::string& s) {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            if (c == ';' || c == '=' || c == '\\') {
                out += '\\';
            }
            out += c;
        }
        return out;
    }

    // Append key=value pair to string
    static void append_kv(std::string& out, const char* key, const std::string& value, bool& first) {
        if (!first) out += ';';
        first = false;
        out += key;
        out += '=';
        out += escape(value);
    }

    // Helper function: find value by dot-separated key
    const toml::Value* find_value(const std::string& key) const {
        if (!valid) return nullptr;
        const toml::Value* current = &config;
        size_t start = 0;
        while (true) {
            size_t dot = key.find('.', start);
            std::string part = key.substr(start, dot - start);

            if (!current->is<toml::Table>()) return nullptr;

            const auto& table = current->as<toml::Table>();
            auto it = table.find(part);
            if (it == table.end()) return nullptr;

            current = &it->second;

            if (dot == std::string::npos) break;
            start = dot + 1;
        }
        return current;
    }

public:
    explicit Config(const char* c_runtime_dir)
        : runtime_dir(c_runtime_dir ? c_runtime_dir : "") 
    {
        if (runtime_dir.empty()) {
            PLOGW << "runtime_dir is empty";
            return;
        }

        std::filesystem::path path = std::filesystem::path(runtime_dir) / "config.toml";
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

    // Get value by key with default
    template<typename T>
    T get_or_default(const std::string& key, const T& default_value, bool use_default_if_empty = false, bool is_path = false) const {
        const toml::Value* val = find_value(key);
        if (!val) return default_value;

        try {
            if constexpr (std::is_same_v<T, std::string>) {
                std::string s = val->as<std::string>();
                if (use_default_if_empty && s.empty()) return default_value;
                // Check if string starts with '.'
                if (is_path && s.starts_with('.')) {
                    // remove '.'
                    s.erase(0, 1);

                    // Check if string starts with '/'
                    // MUST REMOVE '/', or it will cause path error (root directory)
                    if (s.starts_with('/')) {
                        s.erase(0, 1);
                    }

                    // runtime_dir + s
                    s = (std::filesystem::path(runtime_dir) / s).string();
                }
                return s;
            } else {
                return val->as<T>();
            }
        } catch (...) {
            return default_value;
        }
    }

    bool is_valid() const {
        return valid;
    }

    // Get InjectionInfo struct from config
    InjectionInfo get_injection_info() const {
        InjectionInfo info;

        auto get_default_path = [&](const std::string& name) {
            return (std::filesystem::path(runtime_dir) / name).string();
        };

        info.JuiceAgentAPIJarPath = get_or_default<std::string>(
            "JuiceAgent.JuiceAgentAPIJarPath", get_default_path("JuiceAgent-API.jar"), true, true);

        info.JuiceAgentNativePath = get_or_default<std::string>(
            "JuiceAgent.JuiceAgentNativePath", get_default_path("libagent.dll"), true, true);

        info.EntryJarPath = get_or_default<std::string>(
            "Entry.EntryJarPath", get_default_path("Entry.jar"), true, true);

        info.EntryClass = get_or_default<std::string>(
            "Entry.EntryClass", "com.example.Entry", true, false);

        info.EntryMethod = get_or_default<std::string>(
            "Entry.EntryMethod", "start", true, false);

        info.InjectionDir = get_or_default<std::string>(
            "Runtime.InjectionDir", get_default_path("injection"), true, true);

        return info;
    }

    // Serialize InjectionInfo to key=value; format
    static std::string serialize(const InjectionInfo& info) {
        std::string result;
        result.reserve(256);
        bool first = true;

        append_kv(result, "Version", "1", first);

        append_kv(result, "EntryJarPath", info.EntryJarPath, first);
        append_kv(result, "EntryClass", info.EntryClass, first);
        append_kv(result, "EntryMethod", info.EntryMethod, first);

        append_kv(result, "JuiceAgentAPIJarPath", info.JuiceAgentAPIJarPath, first);
        append_kv(result, "JuiceAgentNativePath", info.JuiceAgentNativePath, first);

        append_kv(result, "InjectionDir", info.InjectionDir, first);

        PLOGD << "Serialized args: " << result;
        return result;
    }

    std::string serialize() const {
        return serialize(get_injection_info());
    }
};

// Print InjectionInfo (debug)
inline void print_injection_info(const InjectionInfo& info) {
    PLOGI << "JuiceAgent Injection Info:";
    PLOGI << "  JuiceAgentAPIJarPath: " << info.JuiceAgentAPIJarPath;
    PLOGI << "  JuiceAgentNativePath: " << info.JuiceAgentNativePath;
    PLOGI << "  EntryJarPath: " << info.EntryJarPath;
    PLOGI << "  EntryClass: " << info.EntryClass;
    PLOGI << "  EntryMethod: " << info.EntryMethod;
    PLOGI << "  InjectionDir: " << info.InjectionDir;
}

} // namespace config