#include <modules/Modules.hpp>
#include <JuiceAgent/Logger.hpp>
#include <libagent.hpp>
#include <JuiceAgent/Utils.hpp>

namespace JuiceAgent::Core::Modules {

struct _Config {
    bool Enabled = true;
    std::string InjectionDir;
    std::string JarPath;
    std::string EntryClass;
    std::string EntryMethod;
} Config;

static JuiceAgent::Agent& agent = JuiceAgent::Agent::instance();

static const char* MODULE_CLASS = "cn/xiaozhou233/juiceagent/api/modules/JarLoader";
static const char* MODULE_METHOD = "loadJar";

bool JarLoaderModule::init() {
    if (_initialized) return true;

    PLOGI << "JarLoader init";

    JuiceAgent::Config::Config& cfg = agent.get_config();

    Config.Enabled = cfg.get<bool>("JuiceAgent.Modules.JarLoader.Enabled", false);
    Config.InjectionDir = cfg.get<std::string>("JuiceAgent.Modules.JarLoader.InjectionDir", (agent.get_config().runtime_dir() / "injection").string(), true);
    Config.JarPath = cfg.get<std::string>("JuiceAgent.Modules.JarLoader.JarPath", "./Entry.jar", true);
    Config.EntryClass = cfg.get<std::string>("JuiceAgent.Modules.JarLoader.EntryClass", "Example.Main", false);
    Config.EntryMethod = cfg.get<std::string>("JuiceAgent.Modules.JarLoader.EntryMethod", "start", false);

    PLOGD << ">>> JarLoader Config <<<";
    PLOGD << "Enabled: " << Config.Enabled;
    PLOGD << "InjectionDir: " << Config.InjectionDir;
    PLOGD << "JarPath: " << Config.JarPath;
    PLOGD << "EntryClass: " << Config.EntryClass;
    PLOGD << "EntryMethod: " << Config.EntryMethod;
    PLOGD << ">>> JarLoader Config <<<";
    _initialized = true;
    return true;
}

bool JarLoaderModule::start() {
    if (!_initialized) return false;
    if (_running) return true;

    PLOGI << "JarLoader start";

    if (!Config.Enabled) {
        PLOGI << "JarLoader is disabled";
        return false;
    }

    if (Config.InjectionDir.empty() || Config.JarPath.empty() || Config.EntryClass.empty() || Config.EntryMethod.empty()) {
        PLOGE << "JarLoader config is invalid";
        return false;
    }

    JuiceAgent::Utils::Serializer ser;

    ser.add_kv("InjectionDir", Config.InjectionDir);
    ser.add_kv("JarPath", Config.JarPath);
    ser.add_kv("EntryClass", Config.EntryClass);
    ser.add_kv("EntryMethod", Config.EntryMethod);

    std::string serialized = ser.serialize();

    PLOGD << "Serialized config: " << serialized;

    JNIEnv* env = agent.get_env();
    JuiceAgent::Utils::call_java_impl(env, MODULE_CLASS, MODULE_METHOD, serialized.c_str());

    PLOGI << "JarLoader is running";
    _running = true;
    return true;
}

void JarLoaderModule::stop() {
    if (!_running) return;

    PLOGI << "JarLoader stop";
    _running = false;
}

}