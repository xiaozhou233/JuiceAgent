#include <jni_impl.hpp>
#include <libagent.hpp>

// This function is called by Java to initialize the agent.
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_init
  (JNIEnv *env, jclass , jstring j_runtime_dir) {
    InitLogger();
    
    PLOGI << "Initializing JuiceAgent...";

    // Get runtime directory
    const char* c_runtime_dir = env->GetStringUTFChars(j_runtime_dir, nullptr);
    std::string runtime_dir(c_runtime_dir);
    env->ReleaseStringUTFChars(j_runtime_dir, c_runtime_dir);

    if (runtime_dir.empty()) {
        PLOGE << "Failed to get runtime directory from argument, runtime_dir is empty";
        return JNI_FALSE;
    }
    PLOGI << "JuiceAgent got runtime directory: " << runtime_dir;

    JavaVM* jvm = nullptr;
    jvmtiEnv* jvmti = nullptr;

    // Get Java VM
    env->GetJavaVM(&jvm);
    if (jvm == nullptr) {
        PLOGE << "Failed to get JavaVM instance";
        return JNI_FALSE;
    }

    // Get JVMTI
    jvmtiError err = static_cast<jvmtiError>(jvm->GetEnv(reinterpret_cast<void**>(&jvmti), JVMTI_VERSION_1_2));
    if (err != JVMTI_ERROR_NONE || jvmti == nullptr) {
        PLOGE << "Failed to get JVMTI environment, error: " << err;
        return JNI_FALSE;
    }

    // Invoke EntryPoint
    JuiceAgent::Agent& agent = JuiceAgent::Agent::instance();
    agent.init(jvm, env, jvmti, runtime_dir);

    return JNI_TRUE;    
}
