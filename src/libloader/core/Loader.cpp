#include <jvm/jni.h>
#include <Loader.hpp>
#include <filesystem>
#include <JuiceAgent/Logger.hpp>
#include <JuiceAgent/Config.hpp>
#include <JuiceAgent/ConfigStruct.hpp>

#include <JuiceAgent/JuiceAgent.hpp>

// Preload section
// Check parameters and load the runtime
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
    int version = cfg.get<int>("JuiceAgent.Version");
    if(version != CONFIG_VERSION) {
        spdlog::error("[PreLoad] Config version mismatch! Expected: {}, Got: {}, Please update your config!", CONFIG_VERSION, version);
        return;
    }

    // JarLoader mode check and load jars
    std::string jar_loader_mode = cfg.get<std::string>("JarLoader.Mode");
    if(jar_loader_mode == "BuiltIn") {
        spdlog::info("[PreLoad] JarLoader mode: BuiltIn");

        if(!load_builtin(jvmti)) {
            spdlog::error("[PreLoad] Failed to load built-in jars!");
        }
    } else if(jar_loader_mode == "CustomJar") {
        spdlog::info("[PreLoad] JarLoader mode: CustomJar");

        std::string jar_path = cfg.get<std::string>("JarLoader.JarPath");
        
        if(!load_custom(jvmti, jar_path.c_str())) {
            spdlog::error("[PreLoad] Failed to load custom jar: {}", jar_path);
        }
    } else {
        spdlog::error("[PreLoad] Invalid JarLoader mode: {}", jar_loader_mode);
        return;
    }

    JuiceAgent::Loader::initialize(runtime_dir_new.c_str(), env, jvmti, cfg);
}

void JuiceAgent::Loader::initialize(const char* runtime_dir, JNIEnv* env, jvmtiEnv* jvmti, JuiceAgent::Config::Config& cfg) {
    std::string jar_loader_mode = cfg.get<std::string>("JarLoader.Mode");

    // TODO: Implement this

    // spdlog::info("JuiceAgent initialized!");
}