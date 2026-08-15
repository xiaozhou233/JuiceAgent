#include <Loader.hpp>
#include <JvmAttach.hpp>
#include <JuiceAgent/Logger.hpp>
#include <JuiceAgent/Config.hpp>
#include <JuiceAgent/Utils.hpp>
#include <JuiceAgent/JuiceAgent.hpp>
#include <JuiceAgent/JuiceAgent-API-1.4.1+build.1.hpp>

namespace JuiceAgent::Loader {

static bool invoke_juiceagent_init(JNIEnv* env, const LoaderConfig& config) {
    const char* bootstrap_class = "cn/xiaozhou233/juiceagent/api/JuiceAgentBootstrap";
    const char* method_name = "start";

    std::string args = JuiceAgent::Config::Utils::serialize_loader_config(config);
    spdlog::debug("Invoke args: {}", args);

    if (!JuiceAgent::Utils::call_java_impl(env, bootstrap_class, method_name, args.c_str())) {
        spdlog::error("Failed to init JuiceAgent!");
        return false;
    }

    spdlog::info("JuiceAgentBootstrap.start invoked successfully");
    return true;
}

static void init(const char* runtime_dir, JNIEnv* env, jvmtiEnv* jvmti) {
    if (!runtime_dir) {
        spdlog::warn("Runtime directory is not set!");
    }
    
    // Load configuration
    JuiceAgent::Config::Config cfg(runtime_dir ? runtime_dir : "");
    if (!cfg.is_valid()) {
        spdlog::error("Failed to load configuration!");

        // TODO: Use default values
        // Before impl TODO, must return, or crash will happen
        return;
    }

    // Print configuration
    LoaderConfig config = JuiceAgent::Config::Utils::get_loader_config(cfg);
    JuiceAgent::Config::Utils::print_loader_config(config);

    // Save bytes to file
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
    
    spdlog::debug("JuiceAgent API path: {}", juiceagent_api_path);
    spdlog::debug("JuiceAgent API size: {}", JuiceAgent::Resource::juiceagent_api_bytes_len);
    spdlog::debug("JuiceAgent API sha256: {}", JuiceAgent::Resource::juiceagent_api_bytes_sha256);

    // Load JuiceAgent API to system classloader
    jvmtiError status = jvmti->AddToSystemClassLoaderSearch(juiceagent_api_path.c_str());
    if (status != JVMTI_ERROR_NONE) {
        spdlog::error("Failed to add JuiceAgent API to system classloader! code: {}", static_cast<int>(status));
        return;
    }

    spdlog::info("JuiceAgent API loaded to system classloader!");

    // Init JuiceAgent
    if (!invoke_juiceagent_init(env, config)) {
        spdlog::error("Failed to init JuiceAgent!");
    }
}


void entrypoint(const char* runtime_dir) {
    // Attach to the JVM
    Jvm jvm;
    if (!jvm.attach()) {
        spdlog::error("Failed to attach to JVM!");
        return;
    }

    // Get jvm and jni env
    JNIEnv* env = jvm.get_env();
    auto* jvmti = jvm.get_jvmti();
    if (!env || !jvmti) {
        spdlog::error("Failed to get jni env or jvmti env!");
        return;
    }

    // Init JuiceAgent
    init(runtime_dir, env, jvmti);
}

}
