#include <jni_impl.h>

RetransformClassCacheType RetransformClassCache = {NULL, 0, 0};

static void throwRuntimeException(JNIEnv *env, const char *msg) {
    jclass rte = env->FindClass("java/lang/RuntimeException");
    if (rte != NULL) {
        env->ThrowNew(rte, msg);
    }
}

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

    /* =========================
     * 1. Capture original bytes
     * ========================= */
    if (g_target_internal_name &&
        g_bytecodes == nullptr &&
        strcmp(name, g_target_internal_name) == 0) {

        g_bytecodes_len = class_data_len;
        g_bytecodes = (unsigned char*)malloc(class_data_len);
        memcpy(g_bytecodes, classbytes, class_data_len);

        // IMPORTANT:
        // Do NOT touch new_classbytes here
        // Let JVM continue normally
        PLOGD.printf("[Hook:get] Captured class bytes: %s (%d)", name, class_data_len);
        return;
    }

    /* =========================
     * 2. Apply redefine patch
     * ========================= */
    if (RetransformClassCache.size == 0) return;

    for (int i = 0; i < RetransformClassCache.size; i++) {
        ClassBytesCacheType* entry = &RetransformClassCache.arr[i];
        if (strcmp(entry->name, name) == 0) {

            unsigned char* newBytes = nullptr;
            if (jvmti_env->Allocate(entry->len, &newBytes) != JVMTI_ERROR_NONE) {
                PLOGE.printf("[Hook:set] Allocate failed: %s", name);
                return;
            }

            memcpy(newBytes, entry->bytes, entry->len);
            *new_classbytes = newBytes;
            *new_class_data_len = entry->len;

            PLOGI.printf("[Hook:set] Redefined class: %s (%d)", name, entry->len);

            free(entry->name);
            free(entry->bytes);
            for (int j = i; j < RetransformClassCache.size - 1; j++) {
                RetransformClassCache.arr[j] = RetransformClassCache.arr[j + 1];
            }
            RetransformClassCache.size--;
            return;
        }
    }
}