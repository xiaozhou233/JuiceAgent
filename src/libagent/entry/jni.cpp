#include <jni_common.hpp>

// This function is called by Java to initialize the agent.
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_init
  (JNIEnv *env, jclass , jstring j_runtime_dir) {

    Logger::init("libagent.log");
    
    spdlog::info("Initializing JuiceAgent...");

    // Get runtime directory
    std::string runtime_dir;

    if (j_runtime_dir != nullptr) {
        const char* c_runtime_dir = env->GetStringUTFChars(j_runtime_dir, nullptr);

        if (c_runtime_dir != nullptr) {
            runtime_dir = c_runtime_dir;
            env->ReleaseStringUTFChars(j_runtime_dir, c_runtime_dir);
        }
    }

    if (runtime_dir.empty()) {
        spdlog::warn("Runtime directory is empty");
    }
    spdlog::info("JuiceAgent got runtime directory: {}", runtime_dir);

    JavaVM* jvm = nullptr;
    jvmtiEnv* jvmti = nullptr;

    // Get Java VM
    env->GetJavaVM(&jvm);
    if (jvm == nullptr) {
        spdlog::error("Failed to get JavaVM instance");
        return JNI_FALSE;
    }

    // Get JVMTI
    jvmtiError err = static_cast<jvmtiError>(jvm->GetEnv(reinterpret_cast<void**>(&jvmti), JVMTI_VERSION_1_2));
    if (err != JVMTI_ERROR_NONE || jvmti == nullptr) {
        spdlog::error("Failed to get JVMTI environment, error: {}", static_cast<int>(err));
        return JNI_FALSE;
    }

    // Invoke EntryPoint
    JuiceAgent::Agent& agent = JuiceAgent::Agent::getInstance();
    if (!agent.preload(jvm, env, jvmti, runtime_dir)) {
        spdlog::error("Failed to preload JuiceAgent");
        return JNI_FALSE;
    }
    if (!agent.init()) {
        spdlog::error("Failed to init JuiceAgent");
        return JNI_FALSE;
    }

    return JNI_TRUE;
}