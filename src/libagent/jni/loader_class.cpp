#include <jni_impl.h>
#include <string>
#include <cstring>
#include <algorithm>

/* =========================
 * Global bytecode capture
 * ========================= */
const char* g_target_internal_name = nullptr;
unsigned char* g_bytecodes = nullptr;
jint g_bytecodes_len = 0;

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
 * Exposed: get all loaded classes
 * ========================= */
JNIEXPORT jobjectArray JNICALL
Java_cn_xiaozhou233_juiceloader_JuiceLoader_getLoadedClasses
(JNIEnv* env, jclass) {

    if (!JuiceLoaderNative.jvmti) return nullptr;

    jint count = 0;
    jclass* classes = nullptr;
    if (JuiceLoaderNative.jvmti->GetLoadedClasses(&count, &classes) != JVMTI_ERROR_NONE || count == 0) {
        return env->NewObjectArray(0, env->FindClass("java/lang/Class"), nullptr);
    }

    jclass classClass = env->FindClass("java/lang/Class");
    jobjectArray result = env->NewObjectArray(count, classClass, nullptr);

    for (jint i = 0; i < count; i++) {
        env->SetObjectArrayElement(result, i, classes[i]);
    }

    JuiceLoaderNative.jvmti->Deallocate((unsigned char*)classes);
    return result;
}

/* =========================
 * Get bytecode by jclass
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

    g_target_internal_name = internalName.c_str();
    g_bytecodes = nullptr;
    g_bytecodes_len = 0;

    jvmtiError err = JuiceLoaderNative.jvmti->RetransformClasses(1, &clazz);
    if (err != JVMTI_ERROR_NONE || !g_bytecodes) {
        return nullptr;
    }

    jbyteArray out = env->NewByteArray(g_bytecodes_len);
    env->SetByteArrayRegion(out, 0, g_bytecodes_len, (jbyte*)g_bytecodes);

    free(g_bytecodes);
    g_bytecodes = nullptr;
    g_target_internal_name = nullptr;

    return out;
}

/* =========================
 * Get bytecode by class name
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
        PLOGE.printf("Class not loaded: %s", internal);
        return nullptr;
    }

    return Java_cn_xiaozhou233_juiceloader_JuiceLoader_getClassBytes(
        env, loader_class, clazz
    );
}

/* =========================
 * Get jclass by name (no ClassLoader)
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
        PLOGE.printf("Failed to find loaded class: %s", internal);
    }
    return clazz;
}