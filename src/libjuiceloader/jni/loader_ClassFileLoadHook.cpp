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
        JNIEnv* jni_env,
        jclass class_being_redefined,
        jobject loader,
        const char* name,
        jobject protection_domain,
        jint class_data_len,
        const unsigned char* classbytes,
        jint* new_class_data_len,
        unsigned char** new_classbytes) {

    // ---- capture class bytes ---- //
    if (g_bytecodes_classname != NULL && g_bytecodes == NULL) {
        if (strcmp(name, g_bytecodes_classname) == 0) {
            g_bytecodes_len = class_data_len;
            g_bytecodes = (unsigned char*)malloc(class_data_len);
            memcpy(g_bytecodes, classbytes, class_data_len);

            *new_classbytes = NULL;
            *new_class_data_len = 0;

            PLOGD.printf("[Hook:get] Captured class bytes of %s, len=%d", name, class_data_len);
            return;
        }
    }

    // ---- set class bytes from cache ---- //
    if (RetransformClassCache.size == 0) {
        return;
    }

    for (int i = 0; i < RetransformClassCache.size; i++) {
        ClassBytesCacheType *entry = &RetransformClassCache.arr[i];
        if (strcmp(entry->name, name) == 0) {

            unsigned char *newBytes = nullptr;
            jvmtiError err = jvmti_env->Allocate(entry->len, &newBytes);
            if (err != JVMTI_ERROR_NONE || newBytes == nullptr) {
                PLOGE.printf("[Hook:set] jvmti->Allocate failed for class %s, err=%d", name, err);
                return;
            }

            memcpy(newBytes, entry->bytes, entry->len);
            *new_classbytes = newBytes;
            *new_class_data_len = entry->len;

            PLOGI.printf("[Hook:set] Patched class: %s, new_len=%d", name, entry->len);

            free(entry->name);
            free(entry->bytes);
            for (int j = i; j < RetransformClassCache.size - 1; j++) {
                RetransformClassCache.arr[j] = RetransformClassCache.arr[j + 1];
            }
            RetransformClassCache.size--;
            break;
        }
    }
}
