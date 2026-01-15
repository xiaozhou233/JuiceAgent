#include <jni_impl.h>
#include <string.h>

/* =========================
 * utils
 * ========================= */

static char* get_class_name(JNIEnv* env, jclass clazz) {
    jclass clsClass = env->FindClass("java/lang/Class");
    jmethodID mid = env->GetMethodID(clsClass, "getName", "()Ljava/lang/String;");
    jstring nameStr = (jstring)env->CallObjectMethod(clazz, mid);

    const char* tmp = env->GetStringUTFChars(nameStr, nullptr);
    char* result = _strdup(tmp);
    env->ReleaseStringUTFChars(nameStr, tmp);
    env->DeleteLocalRef(nameStr);
    return result;
}

/* =========================
 * JVMTI global lookup
 * ========================= */

static jclass findLoadedClassByInternalName(
        jvmtiEnv* jvmti,
        const char* internalName
) {
    jint count = 0;
    jclass* classes = nullptr;

    if (jvmti->GetLoadedClasses(&count, &classes) != JVMTI_ERROR_NONE || count == 0) {
        return nullptr;
    }

    jclass result = nullptr;

    for (jint i = 0; i < count; i++) {
        char* sig = nullptr;
        if (jvmti->GetClassSignature(classes[i], &sig, nullptr) == JVMTI_ERROR_NONE && sig) {
            // Lnet/minecraft/client/Minecraft;
            if (sig[0] == 'L') {
                size_t len = strlen(sig);
                if (len > 2 &&
                    strncmp(sig + 1, internalName, len - 2) == 0 &&
                    internalName[len - 2] == '\0') {
                    result = classes[i];
                    jvmti->Deallocate((unsigned char*)sig);
                    break;
                }
            }
            jvmti->Deallocate((unsigned char*)sig);
        }
    }

    jvmti->Deallocate((unsigned char*)classes);
    return result;
}

/* =========================
 * retransform by jclass
 * ========================= */

JNIEXPORT jboolean JNICALL
Java_cn_xiaozhou233_juiceloader_JuiceLoader_retransformClass
(JNIEnv* env, jclass, jclass clazz, jbyteArray classBytes, jint) {

    if (!JuiceLoaderNative.jvmti || !clazz || !classBytes) {
        return JNI_FALSE;
    }

    char* nameDot = get_class_name(env, clazz);
    for (char* p = nameDot; *p; ++p)
        if (*p == '.') *p = '/';

    jbyte* bytes = env->GetByteArrayElements(classBytes, nullptr);
    jint len = env->GetArrayLength(classBytes);

    if (RetransformClassCache.size >= RetransformClassCache.capacity) {
        int newCap = RetransformClassCache.capacity == 0 ? 4 : RetransformClassCache.capacity * 2;
        RetransformClassCache.arr =
            (ClassBytesCacheType*)realloc(RetransformClassCache.arr, newCap * sizeof(ClassBytesCacheType));
        RetransformClassCache.capacity = newCap;
    }

    ClassBytesCacheType* entry =
        &RetransformClassCache.arr[RetransformClassCache.size++];

    entry->name = _strdup(nameDot);
    entry->bytes = (unsigned char*)malloc(len);
    memcpy(entry->bytes, bytes, len);
    entry->len = len;

    env->ReleaseByteArrayElements(classBytes, bytes, JNI_ABORT);

    jclass arr[1] = { clazz };
    jvmtiError err = JuiceLoaderNative.jvmti->RetransformClasses(1, arr);

    free(nameDot);

    if (err != JVMTI_ERROR_NONE) {
        PLOGE.printf("RetransformClasses failed: %d", err);
        return JNI_FALSE;
    }

    PLOGI.printf("[retransformClass] OK: %s (%d)", entry->name, entry->len);
    return JNI_TRUE;
}

/* =========================
 * retransform by name (NO FindClass)
 * ========================= */

JNIEXPORT jboolean JNICALL
Java_cn_xiaozhou233_juiceloader_JuiceLoader_retransformClassByName
(JNIEnv* env, jclass loader_class, jstring className, jbyteArray classBytes, jint length) {

    if (!JuiceLoaderNative.jvmti || !className) {
        return JNI_FALSE;
    }

    const char* utf = env->GetStringUTFChars(className, nullptr);
    char internalName[512];
    strncpy(internalName, utf, sizeof(internalName) - 1);
    internalName[sizeof(internalName) - 1] = 0;

    for (char* p = internalName; *p; ++p)
        if (*p == '.') *p = '/';

    env->ReleaseStringUTFChars(className, utf);

    jclass target =
        findLoadedClassByInternalName(JuiceLoaderNative.jvmti, internalName);

    if (!target) {
        PLOGE.printf("Class not loaded: %s", internalName);
        return JNI_FALSE;
    }

    return Java_cn_xiaozhou233_juiceloader_JuiceLoader_retransformClass(
        env, loader_class, target, classBytes, length
    );
}
