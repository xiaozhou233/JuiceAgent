#include <modules/ModuleRegistry.hpp>
#include <JuiceAgent/Logger.hpp>
#include <libagent.hpp>
#include <JuiceAgent/Utils.hpp>

namespace JuiceAgent::Core::Modules {

struct JarLoaderConfig {
    bool enabled = true;
    std::string injection_dir;
    std::string jar_path;
    std::string entry_class;
    std::string entry_method;
};

static JarLoaderConfig config;
static JuiceAgent::Agent& agent = JuiceAgent::Agent::instance();

static const char* MODULE_CLASS = "cn/xiaozhou233/juiceagent/api/modules/JarLoader";
static const char* MODULE_METHOD = "loadJar";

class JarLoaderModule : public IModule {
public:
    std::string name() const override {
        return "JarLoader";
    }

    bool init() override {
        if (_initialized) {
            return true;
        }

        auto& cfg = agent.get_config();

        config.enabled = cfg.get<bool>(
            "JuiceAgent.Modules.JarLoader.Enabled",
            false
        );

        config.injection_dir = cfg.get<std::string>(
            "JuiceAgent.Modules.JarLoader.InjectionDir",
            (cfg.runtime_dir() / "injection").string(),
            true
        );

        config.jar_path = cfg.get<std::string>(
            "JuiceAgent.Modules.JarLoader.JarPath",
            "./Entry.jar",
            true
        );

        config.entry_class = cfg.get<std::string>(
            "JuiceAgent.Modules.JarLoader.EntryClass",
            "Example.Main",
            false
        );

        config.entry_method = cfg.get<std::string>(
            "JuiceAgent.Modules.JarLoader.EntryMethod",
            "start",
            false
        );

        PLOGI << "JarLoader init";
        _initialized = true;
        return true;
    }

    bool start() override {
        if (!_initialized) {
            return false;
        }

        if (_running || !config.enabled) {
            return true;
        }

        JuiceAgent::Utils::Serializer ser;
        ser.add_kv("InjectionDir", config.injection_dir);
        ser.add_kv("JarPath", config.jar_path);
        ser.add_kv("EntryClass", config.entry_class);
        ser.add_kv("EntryMethod", config.entry_method);

        auto data = ser.serialize();

        JuiceAgent::Utils::call_java_impl(
            agent.get_env(),
            MODULE_CLASS,
            MODULE_METHOD,
            data.c_str()
        );

        PLOGI << "JarLoader start";
        _running = true;
        return true;
    }

    void stop() override {
        if (!_running) {
            return;
        }

        PLOGI << "JarLoader stop";
        _running = false;
    }

private:
    bool _initialized = false;
    bool _running = false;
};

}

JUICEAGENT_REGISTER_MODULE(
    JuiceAgent::Core::Modules::JarLoaderModule,
    JarLoader
)