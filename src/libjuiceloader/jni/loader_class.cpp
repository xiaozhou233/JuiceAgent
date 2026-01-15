#include <jni_impl.h>
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>

/* =========================
 * Global bytecode capture
 * ========================= */
std::string g_target_internal_name_str;
const char* g_target_internal_name = nullptr;
std::vector<unsigned char> g_bytecodes;
RetransformClassCacheType RetransformClassCache = { nullptr, 0, 0 };

/* =========================
 * Utils
 * ========================= */

// Convert java name to internal name: a.b.C -> a/b/C
static void to_internal_name(char* s) {
    for (; *s; ++s) {
        if (*s == '.') *s = '/';
    }
}

/* =========================
 * JVMTI: enumerate loaded classes
 * ========================= */
static jclass findLoadedClassByInternalName(jvmtiEnv* jvmti, const char* internalName) {
    jint count = 0;
    jclass* classes = nullptr;

    if (jvmti->GetLoadedClasses(&count, &classes) != JVMTI_ERROR_NONE || count == 0) {
        return nullptr;
    }

    jclass result = nullptr;
    for (jint i = 0; i < count; i++) {
        char* signature = nullptr;
        if (jvmti->GetClassSignature(classes[i], &signature, nullptr) == JVMTI_ERROR_NONE && signature) {
            // signature format: Lnet/minecraft/xxx;
            size_t len = strlen(signature);
            if (len > 2 && signature[0] == 'L' && signature[len - 1] == ';') {
                if (strncmp(signature + 1, internalName, len - 2) == 0) {
                    result = classes[i];
                    jvmti->Deallocate((unsigned char*)signature);
                    break;
                }
            }
            jvmti->Deallocate((unsigned char*)signature);
        }
    }

    if (classes) {
        jvmti->Deallocate((unsigned char*)classes);
    }
    return result;
}

/* =========================
 * ClassFileLoadHook
 * ========================= */
void JNICALL ClassFileLoadHook(
        jvmtiEnv* jvmti_env,
        JNIEnv*,
        jclass,
        jobject,
        const char* name,
        jobject,
        jint class_data_len,
        const unsigned char* classbytes,
        jint* new_class_data_len,
        unsigned char** new_classbytes) {

    if (!name) return;

    // 捕获目标类字节码
    if (g_target_internal_name && g_bytecodes.empty() &&
        strcmp(name, g_target_internal_name) == 0) {

        g_bytecodes.assign(classbytes, classbytes + class_data_len);
        PLOGD.printf("[Hook:get] Captured class bytes: %s (%d bytes)", name, class_data_len);
        return;
    }

    // 可扩展：这里可以加入类重定义逻辑
}

/* =========================
 * JNI: get class bytes by jclass
 * ========================= */
JNIEXPORT jbyteArray JNICALL
Java_cn_xiaozhou233_juiceloader_JuiceLoader_getClassBytes
(JNIEnv* env, jclass, jclass clazz) {

    if (!JuiceLoaderNative.jvmti || !clazz) return nullptr;

    char* signature = nullptr;
    if (JuiceLoaderNative.jvmti->GetClassSignature(clazz, &signature, nullptr) != JVMTI_ERROR_NONE || !signature) {
        return nullptr;
    }

    // Lxxx; -> xxx
    std::string internalName(signature + 1, strlen(signature) - 2);
    JuiceLoaderNative.jvmti->Deallocate((unsigned char*)signature);

    // 设置全局目标类
    g_target_internal_name_str = internalName;
    g_target_internal_name = g_target_internal_name_str.c_str();
    g_bytecodes.clear();

    // 触发 JVMTI 捕获
    jvmtiError err = JuiceLoaderNative.jvmti->RetransformClasses(1, &clazz);
    if (err != JVMTI_ERROR_NONE) {
        PLOGE.printf("[getClassBytes] RetransformClasses failed: %d", err);
        return nullptr;
    }

    if (g_bytecodes.empty()) {
        PLOGE.printf("[getClassBytes] Failed to capture bytes for: %s", g_target_internal_name);
        return nullptr;
    }

    jbyteArray out = env->NewByteArray((jsize)g_bytecodes.size());
    env->SetByteArrayRegion(out, 0, (jsize)g_bytecodes.size(), (jbyte*)g_bytecodes.data());

    PLOGI.printf("[getClassBytes] Success: %s (%zu bytes)", internalName.c_str(), g_bytecodes.size());

    // 清理全局状态
    g_bytecodes.clear();
    g_target_internal_name = nullptr;

    return out;
}

/* =========================
 * JNI: get class bytes by name
 * ========================= */
JNIEXPORT jbyteArray JNICALL
Java_cn_xiaozhou233_juiceloader_JuiceLoader_getClassBytesByName
(JNIEnv* env, jclass loader_class, jstring className) {

    if (!JuiceLoaderNative.jvmti || !className) return nullptr;

    const char* utf = env->GetStringUTFChars(className, nullptr);
    if (!utf) return nullptr;

    char internal[512];
    strncpy(internal, utf, sizeof(internal) - 1);
    internal[sizeof(internal) - 1] = '\0';
    env->ReleaseStringUTFChars(className, utf);

    to_internal_name(internal);

    jclass clazz = findLoadedClassByInternalName(JuiceLoaderNative.jvmti, internal);
    if (!clazz) {
        PLOGE.printf("[getClassBytesByName] Class not loaded: %s", internal);
        return nullptr;
    }

    return Java_cn_xiaozhou233_juiceloader_JuiceLoader_getClassBytes(env, loader_class, clazz);
}

/* =========================
 * JNI: get jclass by name
 * ========================= */
JNIEXPORT jclass JNICALL
Java_cn_xiaozhou233_juiceloader_JuiceLoader_getClassByName
(JNIEnv* env, jclass, jstring className) {

    if (!JuiceLoaderNative.jvmti || !className) return nullptr;

    const char* utf = env->GetStringUTFChars(className, nullptr);
    if (!utf) return nullptr;

    char internal[512];
    strncpy(internal, utf, sizeof(internal) - 1);
    internal[sizeof(internal) - 1] = '\0';
    env->ReleaseStringUTFChars(className, utf);

    to_internal_name(internal);

    jclass clazz = findLoadedClassByInternalName(JuiceLoaderNative.jvmti, internal);
    if (!clazz) {
        PLOGE.printf("[getClassByName] Failed to find loaded class: %s", internal);
    }
    return clazz;
}
