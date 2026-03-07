#include <JuiceAgent/Logger.hpp>
#include <JuiceAgent/JuiceAgent.h>
#include <tinytoml/toml.h>
#include <string>
#include <fstream>
#include <cstring>

class config
{
private:
    const char* runtime_dir;
    std::string config_path;
    std::ifstream config_file;
    toml::Value config_values;
    InjectionInfoType injection_info;
public:
    config(const char* runtime_dir);
    ~config();

    std::string get_or_default(const char* key, const char* default_value);
    InjectionInfoType get_injection_info();
    void printInjectionInfo();
};

config::config(const char* runtime_dir) {
    // config file path
    std::string config_path = std::string(runtime_dir) + "/config.toml";

    this->runtime_dir = runtime_dir;
    this->config_path = config_path;

    // read config file
    this->config_file.open(config_path);
    if (!this->config_file)
    {
        PLOGE << "Failed to open config file: " << config_path;
    }

    // Parse config file
    toml::ParseResult pr = toml::parse(this->config_file);
    if (!pr.valid())
    {
        PLOGE << "Failed to parse config file: " << config_path;
        return;
    }

    this->config_values = pr.value;
}

std::string config::get_or_default(const char* key, const char* default_value) {
    const toml::Value* value = this->config_values.find(key);

    // key exists and is string
    if (value && value->is<std::string>()) {
        std::string result = value->as<std::string>();

        if (!result.empty()) {
            PLOGD << "Found parameter: " << key << " = " << result;
            return result;
        }

        PLOGD << "Parameter " << key << " is empty, use default: " << default_value;
        return std::string(default_value);
    }

    // key not found
    PLOGD << "Not found parameter: " << key << ", use default: " << default_value;
    return std::string(default_value);
}

InjectionInfoType config::get_injection_info() {
    this->injection_info = {};

    auto copy_from_config = [this](char* dst, size_t dst_size, const char* key, const char* default_value) {
        std::string value = this->get_or_default(key, default_value);
        strncpy_s(dst, dst_size, value.c_str(), _TRUNCATE);
    };

    copy_from_config(this->injection_info.JuiceAgentAPIJarPath, sizeof(injection_info.JuiceAgentAPIJarPath), "JuiceAgentAPIJarPath", "");
    copy_from_config(this->injection_info.JuiceAgentNativePath, sizeof(injection_info.JuiceAgentNativePath), "JuiceAgentNativePath", "");
    copy_from_config(this->injection_info.EntryJarPath, sizeof(injection_info.EntryJarPath), "EntryJarPath", "");
    copy_from_config(this->injection_info.EntryClass, sizeof(injection_info.EntryClass), "EntryClass", "");
    copy_from_config(this->injection_info.EntryMethod, sizeof(injection_info.EntryMethod), "EntryMethod", "");
    copy_from_config(this->injection_info.InjectionDir, sizeof(injection_info.InjectionDir), "InjectionDir", this->runtime_dir);

    return injection_info;
}

void config::printInjectionInfo() {
    PLOGI << "Injection Info:";
    PLOGI << "JuiceAgentAPIJarPath: " << this->injection_info.JuiceAgentAPIJarPath;
    PLOGI << "JuiceAgentNativePath: " << this->injection_info.JuiceAgentNativePath;
    PLOGI << "EntryJarPath: " << this->injection_info.EntryJarPath;
    PLOGI << "EntryClass: " << this->injection_info.EntryClass;
    PLOGI << "EntryMethod: " << this->injection_info.EntryMethod;
    PLOGI << "InjectionDir: " << this->injection_info.InjectionDir;
}