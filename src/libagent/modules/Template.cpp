#include <modules/ModuleRegistry.hpp>
#include <JuiceAgent/Logger.hpp>

namespace JuiceAgent::Core::Modules {

class TemplateModule : public IModule {
public:
    std::string name() const override {
        return "Template";
    }

    bool init() override {
        if (_initialized) {
            return true;
        }

        // load config / prepare resource

        _initialized = true;
        return true;
    }

    bool start() override {
        if (!_initialized) {
            return false;
        }

        if (_running) {
            return true;
        }

        // start logic

        _running = true;
        return true;
    }

    void stop() override {
        if (!_running) {
            return;
        }

        // cleanup logic

        _running = false;
    }

private:
    bool _initialized = false;
    bool _running = false;
};

}

JUICEAGENT_REGISTER_MODULE(
    JuiceAgent::Core::Modules::TemplateModule,
    Template
)