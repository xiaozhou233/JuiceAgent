#include <services.hpp>
#include <event/event_type.hpp>

namespace JuiceAgent::services::Manager {
    void register_events() {
        // PreLoad
        agent.get_eventbus().subscribe<EventPreLoad>(init);

        // Loaded
        agent.get_eventbus().subscribe<EventLoaded>(start);

        // TODO: Stopped

        PLOGI << "Services Manager registered events";
    }

    void init(const EventPreLoad& event) {
        PLOGI << "Initializing services";
        // JarLoader
        JuiceAgent::services::JarLoader::init();
        
    }

    void start(const EventLoaded& event) {
        PLOGI << "Starting services";
        // JarLoader
        JuiceAgent::services::JarLoader::start();
    }

    void stop() {
        
    }
}