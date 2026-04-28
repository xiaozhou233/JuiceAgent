#include <JuiceAgent/Logger.hpp>
#include <Loader.hpp>
#include <JuiceAgent/Config.hpp>
#include <JvmManager.hpp>
#include <JuiceAgent/Utils.hpp>
#include <JuiceAgent/JuiceAgent-API-1.4.1+build.1.hpp>

namespace JuiceAgent::Loader {

// Invoke JuiceAgentBootstrap.start(String) via JNI
bool invoke_juiceagent_init(JNIEnv* env, const LoaderConfig& config) {
    const char* bootstrap_class = "cn/xiaozhou233/juiceagent/api/JuiceAgentBootstrap";
    const char* method_name = "start";
    const char* method_desc = "(Ljava/lang/String;)V";

    // Find Java class
    LocalRef<jclass> cls(env, env->FindClass(bootstrap_class));
    if (!cls.get() || check_and_clear_exception(env, "FindClass")) {
        PLOGE << "Failed to find class: " << bootstrap_class;
        return false;
    }

    // Find static method
    jmethodID method_id = env->GetStaticMethodID(cls, method_name, method_desc);
    if (!method_id || check_and_clear_exception(env, "GetStaticMethodID")) {
        PLOGE << "Failed to find method: " << method_name;
        return false;
    }

    // Serialize InjectionInfo to string
    std::string args = JuiceAgent::Config::Utils::serialize_loader_config(config);
    PLOGD << "Invoke args: " << args;

    // Create Java string
    jstring jArgs = env->NewStringUTF(args.c_str());
    if (!jArgs || check_and_clear_exception(env, "NewStringUTF")) {
        PLOGE << "Failed to create jstring";
        return false;
    }

    // Call the static method
    env->CallStaticVoidMethod(cls, method_id, jArgs);

    // Check for exceptions
    if (check_and_clear_exception(env, "CallStaticVoidMethod")) {
        return false;
    }

    PLOGI << "JuiceAgentBootstrap.start invoked successfully";
    return true;
}

// Entrypoint function for loader
void entrypoint(const char* runtime_dir) {
    if (!runtime_dir) {
        PLOGW << "runtime_dir is null, using empty path";
    }

    // Load configuration
    JuiceAgent::Config::Config cfg(runtime_dir);
    if (!cfg.is_valid()) {
        PLOGW << "Invalid config, using default values";

        // TODO: Use default values
        // Before impl TODO, must return, or crash will happen
        return;
    }

    // Get InjectionInfo
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
        
    PLOGD << "Saved JuiceAgent-API to: " << juiceagent_api_path;
    PLOGD << "JuiceAgent-API size: " << JuiceAgent::Resource::juiceagent_api_bytes_len;
    PLOGD << "JuiceAgent-API SHA256: " << JuiceAgent::Resource::juiceagent_api_bytes_sha256;
    
    // Attach to JVM
    JuiceAgent::Loader::JvmManager::Jvm jvm;
    if (!jvm.attach()) {
        PLOGE << "Attach to JVM failed";
        return;
    }

    JNIEnv* env = jvm.get_env();
    auto* jvmti = jvm.get_jvmti();

    if (!env || !jvmti) {
        PLOGE << "JNIEnv or JVMTI is null";
        return;
    }

    // Inject JuiceAgent jar into system class loader
    jint status = jvmti->AddToSystemClassLoaderSearch(juiceagent_api_path.c_str());
    if (status != JNI_OK) {
        PLOGE << "AddToSystemClassLoaderSearch failed: " << status;
        return;
    }

    PLOGI << "Jar injected successfully: " << juiceagent_api_path;

    // Invoke JuiceAgent initialization
    if (!invoke_juiceagent_init(env, config)) {
        PLOGE << "invoke_juiceagent_init failed";
    }
}

} // namespace JuiceAgent::Loader