#include <modules/Modules.hpp>
#include <JuiceAgent/Logger.hpp>

namespace JuiceAgent::Core::Modules {

bool JarLoaderModule::init() {
    if (_initialized) return true;

    PLOGI << "JarLoader init";
    _initialized = true;
    return true;
}

bool JarLoaderModule::start() {
    if (!_initialized) return false;
    if (_running) return true;

    PLOGI << "JarLoader start";
    _running = true;
    return true;
}

void JarLoaderModule::stop() {
    if (!_running) return;

    PLOGI << "JarLoader stop";
    _running = false;
}

}