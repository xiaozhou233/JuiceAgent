#include <modules/ModuleRegistry.hpp>
#include <JuiceAgent/Logger.hpp>
#include <libagent.hpp>
#include <event/event_type.hpp>

namespace JuiceAgent::Core::Modules {

struct RemapperConfig {
    bool enabled = false;
    std::string JuiceRemapperJarPath;
};

static RemapperConfig config;
static JuiceAgent::Agent& agent = JuiceAgent::Agent::instance();

class RemapperModule : public IModule {
public:
    std::string name() const override {
        return "Remapper";
    }

    bool init() override {
        if (_initialized) {
            return true;
        }

        auto& cfg = agent.get_config();

        config.enabled = cfg.get<bool>(
            "JuiceAgent.Modules.Remapper.Enabled",
            false
        );

        config.JuiceRemapperJarPath = cfg.get<std::string>(
            "JuiceAgent.Modules.Remapper.JuiceRemapperJarPath",
            "./JuiceRemapper.jar",
            true
        );

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

        if (!config.enabled) {
            return true;
        }

        
        // Load JuiceRemapper.jar to jvm
        jint loaded = agent.get_jvmti()->AddToSystemClassLoaderSearch(config.JuiceRemapperJarPath.c_str());
        if (loaded != JNI_OK) {
            PLOGE << "Failed to load JuiceRemapper.jar";
            return false;
        }

        auto& bus = agent.get_eventbus();

        _classFileToken = bus.subscribe<EventClassFileLoadHook>(
            [this](const auto& e) {
                on_class_file_load_hook(e);
            }
        );

        _methodEntryToken = bus.subscribe<EventMethodEntry>(
            [this](const auto& e) {
                on_method_entry_hook(e);
            }
        );

        _methodExitToken = bus.subscribe<EventMethodExit>(
            [this](const auto& e) {
                on_method_exit_hook(e);
            }
        );

        _running = true;
        return true;
    }

    void stop() override {
        if (!_running) {
            return;
        }

        auto& bus = agent.get_eventbus();

        bus.unsubscribe<EventClassFileLoadHook>(_classFileToken);
        bus.unsubscribe<EventMethodEntry>(_methodEntryToken);
        bus.unsubscribe<EventMethodExit>(_methodExitToken);

        _classFileToken = 0;
        _methodEntryToken = 0;
        _methodExitToken = 0;

        _running = false;
    }

    void on_class_file_load_hook(const EventClassFileLoadHook& event) {
        
    }

    void on_method_entry_hook(const EventMethodEntry& event) {
        
    }

    void on_method_exit_hook(const EventMethodExit& event) {
        
    }

private:
    bool _initialized = false;
    bool _running = false;

    EventBus::Token _classFileToken = 0;
    EventBus::Token _methodEntryToken = 0;
    EventBus::Token _methodExitToken = 0;
};

}

JUICEAGENT_REGISTER_MODULE(
    JuiceAgent::Core::Modules::RemapperModule,
    Remapper
)