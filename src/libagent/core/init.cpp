#include <jni_impl.hpp>
#include <libagent.hpp>

// This function is called by Java to initialize the agent.
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceagent_api_JuiceAgent_init
  (JNIEnv *env, jclass loader_class) {
    InitLogger();
    
    PLOGI << "Initializing JuiceAgent...";

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
    agent.init(jvm, env, jvmti);

    return JNI_TRUE;    
}
