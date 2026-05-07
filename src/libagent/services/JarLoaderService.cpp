#include <services.hpp>

namespace JuiceAgent::services::JarLoader {
    static JarLoaderConfig config;

    static const char* MODULE_CLASS = "cn/xiaozhou233/juiceagent/api/modules/JarLoader";
    static const char* MODULE_METHOD = "loadJar";

    void init() {
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

        PLOGI << "JarLoaderService initialized";
    }

    void start() {
        if (!config.enabled) {
            return;
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
        return;
    }
}