#include <services/base.hpp>
#include <services/service_manager.hpp>
#include <event/event_type.hpp>
#include <JuiceAgent/Logger.hpp>
#include <JuiceAgent/Utils.hpp>

namespace JuiceAgent {
namespace Services {

class JarLoaderService : public IService {
public:
    const char* name() const override { return "JarLoaderService"; }

    bool onInitialize() override {
        listen<JarLoaderService, EventLoaded>(
            EventId::Loaded,
            &JarLoaderService::onLoaded
        );

        spdlog::info("[JarLoaderService] Initialized");
        return true;
    }

    void onShutdown() override {
        unlistenAll();
        spdlog::info("[JarLoaderService] Shutdown");
    }

private:
    static constexpr const char* MODULE_CLASS =
        "cn/xiaozhou233/juiceagent/api/modules/JarLoader";
    static constexpr const char* MODULE_METHOD = "loadJar";

    void onLoaded(const EventLoaded& /*event*/) {
        auto& agent = Agent::getInstance();
        auto& cfg = agent.getConfig();

        const std::string injection_dir = cfg.getPath(
            "JarLoader.InjectionDir",
            (cfg.getRuntimeDir() / "injection").string()
        );

        const std::string jar_path = cfg.getPath(
            "JarLoader.JarPath",
            (cfg.getRuntimeDir() / "Entry.jar").string()
        );

        const std::string entry_class = cfg.get<std::string>(
            "JarLoader.EntryClass",
            "Example.Main"
        );

        const std::string entry_method = cfg.get<std::string>(
            "JarLoader.EntryMethod",
            "start"
        );

        JuiceAgent::Utils::Serializer ser;
        ser.add_kv("InjectionDir", injection_dir);
        ser.add_kv("JarPath", jar_path);
        ser.add_kv("EntryClass", entry_class);
        ser.add_kv("EntryMethod", entry_method);

        const std::string data = ser.serialize();
        spdlog::debug("[JarLoaderService] Serialized config: {}", data);

        JNIEnv* env = agent.getJNIEnv();
        if (!env) {
            spdlog::error("[JarLoaderService] JNIEnv is null, cannot load jar");
            return;
        }

        bool result = JuiceAgent::Utils::call_static_java_method(
            env, MODULE_CLASS, MODULE_METHOD, data.c_str());

        if (!result) {
            spdlog::error("[JarLoaderService] Failed to load jar: {}", jar_path);
            return;
        }

        spdlog::info("[JarLoaderService] Loaded jar: {}", jar_path);
    }
};

}
}

JUICEAGENT_REGISTER_SERVICE(::JuiceAgent::Services::JarLoaderService)
