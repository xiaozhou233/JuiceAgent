#include <jvm/jni.h>
#include <Loader.hpp>
#include <filesystem>
#include <JuiceAgent/Logger.hpp>
#include <JuiceAgent/Utils.hpp>
#include <JuiceAgent/Config.hpp>
#include <JuiceAgent/ConfigStruct.hpp>
#include <JuiceAgent/JuiceAgent-API-1.4.1+build.1.hpp>

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

    // TODO: Advanced options
    // Extract JuiceAgent API to temp file
    std::string hash =
        JuiceAgent::Resource::juiceagent_api_bytes_sha256;

    std::string file_name =
        hash.substr(0, 8) + "_" +
        JuiceAgent::Resource::juiceagent_api_bytes_name;

    std::string juiceagent_api_path =
        JuiceAgent::Utils::File::write_to_tempfile(
            JuiceAgent::Resource::juiceagent_api_bytes,
            JuiceAgent::Resource::juiceagent_api_bytes_len,
            file_name
        );
    
    // Debug
    spdlog::debug("JuiceAgent API path: {}", juiceagent_api_path);
    spdlog::debug("JuiceAgent API size: {}", JuiceAgent::Resource::juiceagent_api_bytes_len);
    spdlog::debug("JuiceAgent API sha256: {}", JuiceAgent::Resource::juiceagent_api_bytes_sha256);

    // Load JuiceAgent API
    jvmtiError status = jvmti->AddToSystemClassLoaderSearch(juiceagent_api_path.c_str());
    if (status != JVMTI_ERROR_NONE) {
        spdlog::error("Failed to add JuiceAgent API to system classloader! code: {}", static_cast<int>(status));
        return;
    }

    spdlog::info("JuiceAgent API loaded to system classloader!");

    // Init JuiceAgent via JuiceAgent API
    JuiceAgent::Loader::initialize(runtime_dir_new.c_str(), env, jvmti);
}

void JuiceAgent::Loader::initialize(const char* runtime_dir, JNIEnv* env, jvmtiEnv* jvmti) {
    const char* bootstrap_class = "cn/xiaozhou233/juiceagent/api/JuiceAgentBootstrap";
    const char* method_name = "start";

    // Read JuiceAgent config
    JuiceAgent::Config::Config cfg(runtime_dir ? runtime_dir : "");
    if (!cfg.is_valid()) {
        spdlog::error("JuiceAgent configuration is invalid!");
        return;
    }

    // TODO: Refactor config
    LoaderConfig config = JuiceAgent::Config::Utils::get_loader_config(cfg);
    JuiceAgent::Config::Utils::print_loader_config(config);

    // Serialize InjectionInfo to string
    std::string args = JuiceAgent::Config::Utils::serialize_loader_config(cfg, config);
    spdlog::debug("Serialized config: {}", args);

    bool result = JuiceAgent::Utils::call_java_impl(env, bootstrap_class, method_name, args.c_str());
    if (!result) {
        spdlog::error("Failed to initialize JuiceAgent!");
        return;
    }

    spdlog::info("JuiceAgent initialized!");
}