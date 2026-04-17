#pragma once

#include <filesystem>
#include <JuiceAgent/Logger.hpp>

namespace JuiceAgent::Config
{
    class Config {
        private:
            std::filesystem::path config_dir;
        public:
            Config(std::filesystem::path config_dir) : config_dir(config_dir){
                // Check if config_dir exists
                if (!std::filesystem::exists(config_dir)){
                    PLOGE << "Config directory does not exist" << config_dir;
                    return;
                }

                std::filesystem::path config_file = config_dir / "config.toml";
                // Check if config file exists
                if (!std::filesystem::exists(config_file)){
                    PLOGE << "Config file does not exist" << config_file;
                    return;
                }

                
            }
    };
} // namespace JuiceAgent::Config
