#pragma once

#include <string>
#include <JuiceAgent.hpp>
#include <event/event_type.hpp>

namespace JuiceAgent::services
{
    static JuiceAgent::Agent& agent = JuiceAgent::Agent::instance();

    namespace Manager {
        void register_events();
        void init(const EventPreLoad&);
        void start(const EventLoaded&);
        void stop();
    }

    namespace JarLoader
    {
        struct JarLoaderConfig {
            bool enabled = false;
            std::string injection_dir;
            std::string jar_path;
            std::string entry_class;
            std::string entry_method;
        };

        void init();
        void start();
    } // namespace JarLoader
    
    namespace Bytecode
    {
        void init();
        void start();
    }
} // namespace JuiceAgent::services