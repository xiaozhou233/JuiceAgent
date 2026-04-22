#pragma once

#include <modules/IModule.hpp>

namespace JuiceAgent::Core::Modules {

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

}