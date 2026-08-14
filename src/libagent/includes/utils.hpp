#pragma once

#include <JuiceAgent.hpp>
#include <cstring>

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

// Search all loaded classes for one whose internal name (e.g. "a/b/C") matches.
// Returns a jclass reference owned by JVMTI (must not be deleted); nullptr if not found.
inline jclass find_class_by_internal_name(jvmtiEnv* jvmti, const std::string& internal_name) {
    if (!jvmti || internal_name.empty()) return nullptr;

    jint count = 0;
    jclass* classes = nullptr;
    if (jvmti->GetLoadedClasses(&count, &classes) != JVMTI_ERROR_NONE || count == 0) {
        return nullptr;
    }

    jclass result = nullptr;
    for (jint i = 0; i < count; i++) {
        char* signature = nullptr;
        if (jvmti->GetClassSignature(classes[i], &signature, nullptr) != JVMTI_ERROR_NONE || !signature) {
            continue;
        }

        size_t len = strlen(signature);
        // Signature format: "La/b/C;"
        if (len > 2 && signature[0] == 'L' && signature[len - 1] == ';' &&
            internal_name.size() == len - 2 &&
            memcmp(signature + 1, internal_name.data(), len - 2) == 0) {
            result = classes[i];
            jvmti->Deallocate(reinterpret_cast<unsigned char*>(signature));
            break;
        }

        jvmti->Deallocate(reinterpret_cast<unsigned char*>(signature));
    }

    jvmti->Deallocate(reinterpret_cast<unsigned char*>(classes));
    return result;
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