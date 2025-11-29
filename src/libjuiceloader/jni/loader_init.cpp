#include <jni_impl.h>

JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceloader_JuiceLoader_init 
(JNIEnv *env, jclass loader_class) {
    PLOGI << "Invoked!";

    // Get JavaVM
    JavaVM* jvm = nullptr;
    env->GetJavaVM(&jvm);
    if (jvm == nullptr) {
        PLOGE << "Failed to get JavaVM instance";
        return JNI_FALSE;
    }

    // Get JVMTI
    jvmtiEnv* jvmti = nullptr;
    jvmtiError err = static_cast<jvmtiError>(jvm->GetEnv(reinterpret_cast<void**>(&jvmti), JVMTI_VERSION_1_2));
    if (err != JVMTI_ERROR_NONE || jvmti == nullptr) {
        PLOGE << "Failed to get JVMTI environment, error: " << err;
        return JNI_FALSE;
    }

    int result = InitJuiceLoader(env, jvmti);
    if (result != 0) {
        PLOGE << "Failed to initialize JuiceLoader, error code: " << result;
        return JNI_FALSE;
    }

    PLOGI << "JuiceLoader initialized successfully";
    return JNI_TRUE;
}
