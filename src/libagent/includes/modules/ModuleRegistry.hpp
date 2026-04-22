#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <string>
#include <modules/IModule.hpp>

namespace JuiceAgent::Core::Modules {

class ModuleRegistry {
public:
    using Factory = std::function<std::unique_ptr<IModule>()>;

    struct Entry {
        std::string name;
        Factory factory;
    };

    static ModuleRegistry& instance() {
        static ModuleRegistry inst;
        return inst;
    }

    bool add(const std::string& name, Factory factory) {
        for (const auto& item : _entries) {
            if (item.name == name) {
                return false;
            }
        }

        _entries.push_back({name, std::move(factory)});
        return true;
    }

    const std::vector<Entry>& entries() const {
        return _entries;
    }

private:
    std::vector<Entry> _entries;
};

}

#define JUICEAGENT_REGISTER_MODULE(TYPE, NAME)                \
namespace {                                                  \
    const bool registered_##NAME =                           \
        JuiceAgent::Core::Modules::ModuleRegistry::instance().add( \
            #NAME,                                           \
            []() -> std::unique_ptr<JuiceAgent::Core::Modules::IModule> { \
                return std::make_unique<TYPE>();             \
            }                                                \
        );                                                   \
}

