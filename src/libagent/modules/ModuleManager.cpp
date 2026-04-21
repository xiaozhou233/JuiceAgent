#include <modules/ModuleManager.hpp>
#include <JuiceAgent/Logger.hpp>
#include <modules/Modules.hpp>

namespace JuiceAgent::Core::Modules {

void ModuleManager::register_module(std::unique_ptr<IModule> module) {
    if (!module) return;

    PLOGI << "Register module: " << module->name();
    _modules.push_back(std::move(module));
}

bool ModuleManager::init_all() {
    for (auto& module : _modules) {
        PLOGI << "Init module: " << module->name();

        if (!module->init()) {
            PLOGE << "Failed to init module: " << module->name();
            return false;
        }
    }
    return true;
}

bool ModuleManager::start_all() {
    for (auto& module : _modules) {
        PLOGI << "Start module: " << module->name();

        if (!module->start()) {
            PLOGE << "Failed to start module: " << module->name();
            return false;
        }
    }
    return true;
}

void ModuleManager::stop_all() {
    for (auto it = _modules.rbegin(); it != _modules.rend(); ++it) {
        (*it)->stop();
    }
}

IModule* ModuleManager::find_module(const std::string& name) {
    for (auto& module : _modules) {
        if (module->name() == name) {
            return module.get();
        }
    }
    return nullptr;
}

bool ModuleManager::loadAllRegisteredModules() {
    auto moduleNames = ModuleRegistry::getRegisteredModuleNames();
    for (const auto& name : moduleNames) {
        PLOGI << "Loading registered module: " << name;
        auto module = ModuleRegistry::createModule(name);
        if (module) {
            register_module(std::move(module));
        } else {
            PLOGE << "Failed to create module: " << name;
            return false;
        }
    }
    return true;
}

}