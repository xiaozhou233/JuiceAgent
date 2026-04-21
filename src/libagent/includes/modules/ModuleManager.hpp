#pragma once

#include <memory>
#include <vector>
#include <string>
#include <modules/IModule.hpp>
#include <JuiceAgent/Config.hpp>

namespace JuiceAgent::Core::Modules {

class ModuleManager {
public:
    ModuleManager() = default;
    ~ModuleManager() = default;

    void register_module(std::unique_ptr<IModule> module);

    bool init_all();
    bool start_all();
    void stop_all();

    IModule* find_module(const std::string& name);

    bool loadAllRegisteredModules();

private:
    std::vector<std::unique_ptr<IModule>> _modules;
};

}