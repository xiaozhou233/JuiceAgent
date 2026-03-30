#pragma once

#include <JuiceAgent/Logger.hpp>
#include <JuiceAgent/JuiceAgent.hpp>
#include <tinytoml/toml.h>
#include <filesystem>
#include <fstream>

namespace config
{
    class Config {
    private:
        std::string runtime_dir;
        toml::Value config{};
        bool valid = false;

    public:
        explicit Config(const char* c_runtime_dir) 
        : runtime_dir(c_runtime_dir) {
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

        template<typename T>
        T get_or_default(const std::string& key, const T& default_value) const {
            if (!valid) return default_value;

            const toml::Value* current = &config;

            size_t start = 0;
            while (true) {
                size_t dot = key.find('.', start);
                std::string part = key.substr(start, dot - start);

                if (!current->is<toml::Table>()) {
                    return default_value;
                }

                auto it = current->as<toml::Table>().find(part);
                if (it == current->as<toml::Table>().end()) {
                    return default_value;
                }

                current = &it->second;

                if (dot == std::string::npos) break;
                start = dot + 1;
            }

            try {
                return current->as<T>();
            } catch (...) {
                return default_value;
            }
        }

        bool is_valid() const {
            return valid;
        }

        InjectionInfo get_injection_info() {
            InjectionInfo info;
            auto get_default_path = [&](const std::string& name) -> std::string {
                return (std::filesystem::path(runtime_dir) / name).string();
            };

            // [JuiceAgent]
            // -- APIJarPath = "runtime_dir/JuiceAgent.jar"
            info.JuiceAgentAPIJarPath = get_or_default<std::string>("JuiceAgent.JuiceAgentAPIJarPath", get_default_path("JuiceAgent.jar"));
            // -- JuiceAgentNativePath = "runtime_dir/libagent.dll/.so"
            info.JuiceAgentNativePath = get_or_default<std::string>("JuiceAgent.JuiceAgentNativePath", get_default_path("libagent.dll"));

            // [Entry]
            // -- EntryJarPath = "runtime_dir/entry.jar"
            info.EntryJarPath = get_or_default<std::string>("Entry.EntryJarPath", get_default_path("entry.jar"));
            // -- EntryClass = "com.example.Entry"
            info.EntryClass = get_or_default<std::string>("Entry.EntryClass", "com.example.Entry");
            // -- EntryMethod = "start"
            info.EntryMethod = get_or_default<std::string>("Entry.EntryMethod", "start");

            // [Runtime]
            // -- InjectionDir = "runtime_dir/injection"
            info.InjectionDir = get_or_default<std::string>("Runtime.InjectionDir", get_default_path("injection"));

            return info;
        }
    };
    
    void print_injection_info(const InjectionInfo& info) {
        PLOGI << "JuiceAgent Injection Info:";
        PLOGI << "  JuiceAgentAPIJarPath: " << info.JuiceAgentAPIJarPath;
        PLOGI << "  JuiceAgentNativePath: " << info.JuiceAgentNativePath;
        PLOGI << "  EntryJarPath: " << info.EntryJarPath;
        PLOGI << "  EntryClass: " << info.EntryClass;
        PLOGI << "  EntryMethod: " << info.EntryMethod;
        PLOGI << "  InjectionDir: " << info.InjectionDir;
    }
}