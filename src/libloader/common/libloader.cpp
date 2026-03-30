#pragma once

#include <JuiceAgent/Logger.hpp>
#include <libloader.hpp>
#include <config.hpp>
#include <jvm.hpp>

namespace libloader {

namespace {
    // Check for JNI exceptions and clear them
    bool check_and_clear_exception(JNIEnv* env, const char* context) {
        if (env->ExceptionCheck()) {
            PLOGE << "JNI Exception occurred at: " << context;
            env->ExceptionDescribe();
            env->ExceptionClear();
            return true;
        }
        return false;
    }

    // RAII wrapper for local JNI references
    template<typename T>
    class LocalRef {
    private:
        JNIEnv* env;
        T ref;
    public:
        LocalRef(JNIEnv* env, T ref) : env(env), ref(ref) {}
        ~LocalRef() {
            if (ref) env->DeleteLocalRef(ref);
        }
        T get() const { return ref; }
        operator T() const { return ref; }
    };
}

// Invoke JuiceAgentBootstrap.start(String) via JNI
bool invoke_juiceagent_init(JNIEnv* env, const InjectionInfo& info) {
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
    std::string args = config::Config::serialize(info);
    PLOGD << "Invoke args: " << args;

    // Create Java string
    LocalRef<jstring> jArgs(env, env->NewStringUTF(args.c_str()));
    if (!jArgs.get() || check_and_clear_exception(env, "NewStringUTF")) {
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
    config::Config cfg(runtime_dir);
    if (!cfg.is_valid()) {
        PLOGW << "Invalid config, using default values";
    }

    // Get InjectionInfo
    InjectionInfo info = cfg.get_injection_info();
    config::print_injection_info(info);

    // Validate essential paths
    if (info.JuiceAgentAPIJarPath.empty()) {
        PLOGE << "JuiceAgentAPIJarPath is empty, abort injection";
        return;
    }

    // Attach to JVM
    jvm::Jvm jvm;
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
    jint status = jvmti->AddToSystemClassLoaderSearch(info.JuiceAgentAPIJarPath.c_str());
    if (status != JNI_OK) {
        PLOGE << "AddToSystemClassLoaderSearch failed: " << status;
        return;
    }

    PLOGI << "Jar injected successfully: " << info.JuiceAgentAPIJarPath;

    // Invoke JuiceAgent initialization
    if (!invoke_juiceagent_init(env, info)) {
        PLOGE << "invoke_juiceagent_init failed";
    }
}

} // namespace libloader