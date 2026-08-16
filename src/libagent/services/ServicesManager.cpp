#include <services.hpp>
#include <event/event_type.hpp>

namespace JuiceAgent::services::Manager {
    void register_events() {
        // PreLoad
        agent().get_eventbus().subscribe<EventPreLoad>(init);

        // Loaded
        agent().get_eventbus().subscribe<EventLoaded>(start);

        // TODO: Stopped

        spdlog::info("Services Manager registered events");
    }

    void init(const EventPreLoad& event) {
        spdlog::info("Initializing services");
        // JarLoader
        JuiceAgent::services::JarLoader::init();
        
        // Bytecode
        JuiceAgent::services::Bytecode::init();
    }

    void start(const EventLoaded& event) {
        spdlog::info("Starting services");
        // JarLoader
        JuiceAgent::services::JarLoader::start();

        // Bytecode
        JuiceAgent::services::Bytecode::start();
    }

    void stop() {
        
    }
}