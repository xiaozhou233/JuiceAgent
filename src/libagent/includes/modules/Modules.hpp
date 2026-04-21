#pragma once

#include <modules/IModule.hpp>
#include <functional>
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

namespace JuiceAgent::Core::Modules {

class ModuleRegistry {
public:
    using Creator = std::function<std::unique_ptr<IModule>()>;

    static void registerModule(const std::string& name, Creator creator) {
        getRegistry()[name] = creator;
    }

    static std::unique_ptr<IModule> createModule(const std::string& name) {
        auto& reg = getRegistry();
        auto it = reg.find(name);
        if (it != reg.end()) {
            return it->second();
        }
        return nullptr;
    }

    static std::vector<std::string> getRegisteredModuleNames() {
        std::vector<std::string> names;
        auto& reg = getRegistry();
        for (const auto& pair : reg) {
            names.push_back(pair.first);
        }
        return names;
    }

private:
    static std::unordered_map<std::string, Creator>& getRegistry() {
        static std::unordered_map<std::string, Creator> registry;
        return registry;
    }
};

class JarLoaderModule : public IModule {
public:
    JarLoaderModule() = default;
    ~JarLoaderModule() override = default;

    std::string name() const override {
        return "JarLoader";
    }

    bool init() override;
    bool start() override;
    void stop() override;

private:
    bool _initialized = false;
    bool _running = false;
};

#define REGISTER_MODULE(ModuleClass) \
    static bool _registered_##ModuleClass = []() { \
        ModuleRegistry::registerModule(#ModuleClass, []() { return std::make_unique<ModuleClass>(); }); \
        return true; \
    }()

} // namespace JuiceAgent::Core::Modules