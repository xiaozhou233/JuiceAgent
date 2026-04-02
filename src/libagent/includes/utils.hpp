#pragma once

#include <libagent.hpp>

inline bool check_env(JuiceAgent::Agent& agent) {
    if (agent.get_jvm() == nullptr) {
        PLOGE << "Failed to get JavaVM instance";
        return false;
    }
    if (agent.get_jvmti() == nullptr) {
        PLOGE << "Failed to get JVMTI environment";
        return false;
    }
    return true;
}

inline std::string get_class_name(JNIEnv* env, jclass clazz) {
    if (!env || !clazz) return "";

    jclass clsClass = env->FindClass("java/lang/Class");
    if (!clsClass) return "";

    jmethodID mid = env->GetMethodID(clsClass, "getName", "()Ljava/lang/String;");
    if (!mid) {
        env->DeleteLocalRef(clsClass);
        return "";
    }

    jstring nameStr = static_cast<jstring>(env->CallObjectMethod(clazz, mid));
    std::string result;

    if (nameStr) {
        const char* tmp = env->GetStringUTFChars(nameStr, nullptr);
        if (tmp) {
            result = tmp; // copy to std::string
            env->ReleaseStringUTFChars(nameStr, tmp);
        }
        env->DeleteLocalRef(nameStr);
    }

    env->DeleteLocalRef(clsClass);
    return result;
}