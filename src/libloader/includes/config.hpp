#include <string>
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <tinytoml/toml.h>
#include <plog/Log.h>

namespace config
{
    class Config {
    private:
        std::string config_path;
        toml::Value config;

    public:
        Config(const std::string& config_path)
            : config_path(config_path)
        {
            PLOGD << "[Config] Loading config file: " << config_path;

            std::ifstream file(config_path);
            if (!file.is_open()) {
                PLOGE << "[Config] Failed to open config file: " << config_path;
                throw std::runtime_error("Failed to open config file: " + config_path);
            }

            toml::ParseResult pr = toml::parse(file);

            if (!pr.valid()) {
                PLOGE << "[Config] Failed to parse config file: " << config_path
                      << " | Reason: " << pr.errorReason;

                throw std::runtime_error(
                    "Failed to parse config file: " + config_path + "\n" + pr.errorReason
                );
            }

            config = pr.value;

            PLOGI << "[Config] Successfully loaded config file: " << config_path;
        }

        template<typename T>
        T get_or_default(const std::string& key, const T& default_value) const
        {
            const toml::Value* value = config.find(key);

            if (!value) {
                PLOGW << "[Config] Key not found: " << key
                      << " | Using default value";
                return default_value;
            }

            if (!value->is<T>()) {
                PLOGE << "[Config] Type mismatch for key: " << key
                      << " | Expected type does not match | Using default value";
                return default_value;
            }

            PLOGD << "[Config] Read key: " << key;

            return value->as<T>();
        }

        std::string get_path_or_default(const std::string& key, const std::string& def) const
        {
            std::string result = get_or_default<std::string>(key, def);

            std::filesystem::path p(result);

            if (p.is_relative()) {
                std::filesystem::path full =
                    std::filesystem::path(config_path).parent_path() / p;

                PLOGD << "[Config] Resolved relative path: " << result
                      << " -> " << full.string();

                return full.string();
            }

            PLOGD << "[Config] Using absolute path: " << result;

            return result;
        }
    };
}