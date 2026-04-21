#include <modules/Modules.hpp>
#include <JuiceAgent/Logger.hpp>
#include <libagent.hpp>

namespace JuiceAgent::Core::Modules {

struct _ExampleConfig {
    bool Enabled = true;
    std::string Message = "Hello from Example Module!";
} ExampleConfig;

static JuiceAgent::Agent& agent = JuiceAgent::Agent::instance();

bool ExampleModule::init() {
    if (_initialized) return true;
    
    // Coding here

    _initialized = true;
    return true;
}

bool ExampleModule::start() {
    if (!_initialized) return false;
    if (_running) return true;

    // Coding here

    _running = true;
    return true;
}

void ExampleModule::stop() {
    if (!_running) return;
    
    // Coding here

    _running = false;
}

} // namespace JuiceAgent::Core::Modules

namespace {
    bool registerExample = []() {
        JuiceAgent::Core::Modules::ModuleRegistry::registerModule("Example", []() {
            return std::make_unique<JuiceAgent::Core::Modules::ExampleModule>();
        });
        return true;
    }();
} 
