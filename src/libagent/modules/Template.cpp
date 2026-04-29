#include <modules/ModuleBase.hpp>
#include <JuiceAgent/Logger.hpp>

namespace JuiceAgent::Core::Modules {

class TemplateModule : public ModuleBase {
public:
    std::string name() const override {
        return "Template";
    }

protected:
    bool on_init() override {
        // load config / prepare resource
        return true;
    }

    bool on_start() override {
        // start module
        return true;
    }

    void on_stop() override {
        // stop module
    }
};

}