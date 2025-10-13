#include "./GlobalUtils.h"

// Get JavaVM
jint GetCreatedJVM(JavaVM **out_jvm) {
    if (!out_jvm) return JNI_ERR;
    *out_jvm = NULL;

    jint res = JNI_GetCreatedJavaVMs(out_jvm, 1, NULL);
    if (res != JNI_OK || *out_jvm == NULL) {
        MessageBoxA(NULL, "JNI_GetCreatedJavaVMs failed", "Error", MB_OK);
        return res;
    }
    return JNI_OK;
}

// Get JNIEnv
jint GetJNIEnv(JavaVM *jvm, JNIEnv **out_env) {
    if (!jvm || !out_env) return JNI_ERR;
    *out_env = NULL;

    void *env = NULL;
    jint res = (*jvm)->GetEnv(jvm, &env, JNI_VERSION_1_8);
    if (res == JNI_OK && env != NULL) {
        *out_env = (JNIEnv *)env;
        return JNI_OK;
    }

    // Attach current thread
    res = (*jvm)->AttachCurrentThread(jvm, &env, NULL);
    if (res != JNI_OK || env == NULL) {
        return res;
    }
    *out_env = (JNIEnv *)env;
    return JNI_OK;
}

// Get JVMTI
jint GetJVMTI(JavaVM *jvm, jvmtiEnv **out_jvmti) {
    if (!jvm || !out_jvmti) return JNI_ERR;
    *out_jvmti = NULL;

    jint res = (*jvm)->GetEnv(jvm, (void**)out_jvmti, JVMTI_VERSION_1_2);
    if (res == JNI_OK && *out_jvmti != NULL) {
        return JNI_OK;
    }
    
    *out_jvmti = NULL;
    return res;
}