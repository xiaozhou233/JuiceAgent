#include <jvm/jni.h>
#include <Loader.hpp>
#include <filesystem>
#include <JuiceAgent/Logger.hpp>
#include <JuiceAgent/Config.hpp>
#include <JuiceAgent/Utils.hpp>
#include <JuiceAgent/JuiceAgent.hpp>
#include <vector>

// Preload section
// Check parameters and load the runtime
//
// NOTE: libloader is reflectively loaded, so its unwind tables are never
// registered with the OS - a C++ exception thrown here could not be caught and
// would crash the target process. Everything below must stay exception-free and
// report problems with spdlog plus an early return.
void JuiceAgent::Loader::preload(const char* runtime_dir, JNIEnv* env, jvmtiEnv* jvmti) {
    std::string runtime_dir_new;

    // Check parameters
    if(!runtime_dir) {
        spdlog::warn("[PreLoad] Runtime directory is not specified!");

        runtime_dir_new = std::filesystem::current_path().string();

        spdlog::warn( "[PreLoad] Using default runtime directory: {}", runtime_dir_new);
    } else {
        runtime_dir_new = runtime_dir;
    }

    spdlog::info("[PreLoad] Runtime directory: {}", runtime_dir_new);

    spdlog::trace("[PreLoad] Loading config...");
    JuiceAgent::Config::Config cfg(runtime_dir ? runtime_dir : "");
    if(!cfg.isValid()) {
        spdlog::error("[PreLoad] Failed to load config!");
        return;
    }

    // Check config version
    if(!cfg.contains("JuiceAgent.Version")) {
        spdlog::error("[PreLoad] Missing required config key: JuiceAgent.Version");
        return;
    }
    const int version = cfg.get<int>("JuiceAgent.Version", -1);
    if(version != CONFIG_VERSION) {
        spdlog::error("[PreLoad] Config version mismatch! Expected: {}, Got: {}, Please update your config!", CONFIG_VERSION, version);
        return;
    }

    // JarLoader mode check and load jars
    if(!cfg.contains("JarLoader.Mode")) {
        spdlog::error("[PreLoad] Missing required config key: JarLoader.Mode");
        return;
    }
    const std::string jar_loader_mode = cfg.get<std::string>("JarLoader.Mode", "");

    if(jar_loader_mode == "BuiltIn") {
        spdlog::info("[PreLoad] JarLoader mode: BuiltIn");

        if(!load_builtin(jvmti)) {
            spdlog::error("[PreLoad] Failed to load built-in jars!");
            return;
        }
    } else if(jar_loader_mode == "CustomJar") {
        spdlog::info("[PreLoad] JarLoader mode: CustomJar");

        const std::string jar_path = cfg.getPath("JarLoader.JarPath", "");
        if(jar_path.empty()) {
            spdlog::error("[PreLoad] Missing required config key: JarLoader.JarPath (CustomJar mode)");
            return;
        }

        if(!load_custom(jvmti, jar_path.c_str())) {
            spdlog::error("[PreLoad] Failed to load custom jar: {}", jar_path);
            return;
        }
    } else {
        spdlog::error("[PreLoad] Invalid JarLoader mode: {}", jar_loader_mode);
        return;
    }

    JuiceAgent::Loader::initialize(runtime_dir_new.c_str(), env, jvmti, cfg);
}

void JuiceAgent::Loader::initialize(const char* runtime_dir, JNIEnv* env, jvmtiEnv* jvmti, JuiceAgent::Config::Config& cfg) {
    const std::string jar_loader_mode = cfg.get<std::string>("JarLoader.Mode", "");

    if(jar_loader_mode == "BuiltIn") {
        const char* bootstrap_class = "cn/xiaozhou233/juiceagent/api/JuiceAgentBootstrap";
        const char* method_name = "start";

        JuiceAgent::Utils::Serializer ser;
        ser.add_kv("Version", cfg.get<int>("JuiceAgent.Version", -1));
        ser.add_kv("JuiceAgentNativeLibraryPath", cfg.getPath("JuiceAgent.JuiceAgentNativeLibraryPath", (cfg.getRuntimeDir()/"libagent.dll").string()));
        ser.add_kv("RuntimeDir", cfg.getRuntimeDir().string());

        std::string args = ser.serialize(); 
        spdlog::debug("Serialized config: {}", args);

        bool result = JuiceAgent::Utils::call_static_java_method(env, bootstrap_class, method_name, args.c_str());
        if (!result) {
            spdlog::error("Failed to initialize JuiceAgent! (call_static_java_method)");
            return;
        }
    } else if(jar_loader_mode == "CustomJar") {
        spdlog::info("[Initialize] JarLoader mode: CustomJar");

        const std::string entry_class = cfg.get<std::string>("JarLoader.EntryClass", "");
        const std::string entry_method = cfg.get<std::string>("JarLoader.EntryMethod", "");
        if(entry_class.empty() || entry_method.empty()) {
            spdlog::error("[Initialize] Missing required config key: JarLoader.EntryClass / JarLoader.EntryMethod (CustomJar mode)");
            return;
        }

        spdlog::debug("[Initialize] Entry class: {}, Entry method: {}", entry_class, entry_method);
        bool result = JuiceAgent::Utils::call_static_java_method(env, entry_class.c_str(), entry_method.c_str());
        if (!result) {
            spdlog::error("Failed to invoke custom jar entry! (call_static_java_method)");
            return;
        }
    } else {
        spdlog::error("[Initialize] Invalid JarLoader mode: {}", jar_loader_mode);
        return;
    }

    spdlog::info("JuiceAgent initialized!");
}
