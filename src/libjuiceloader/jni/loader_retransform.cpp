#include <jni_impl.h>

static char* get_class_name(JNIEnv* env, jclass clazz) {
    jclass clsClass = env->FindClass("java/lang/Class");
    jmethodID mid_getName = env->GetMethodID(clsClass, "getName", "()Ljava/lang/String;");
    jstring nameStr = (jstring)env->CallObjectMethod(clazz, mid_getName);

    const char* temp = env->GetStringUTFChars(nameStr, NULL);
    char* result = _strdup(temp);
    env->ReleaseStringUTFChars(nameStr, temp);
    env->DeleteLocalRef(nameStr);
    return result;
}

JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceloader_JuiceLoader_retransformClass
(JNIEnv *env, jclass loader_class, jclass clazz, jbyteArray classBytes, jint length) {
    if (!JuiceLoaderNative.jvmti) {
        PLOGE << "JuiceLoaderNative.jvmti is NULL! (retransformClass)";
        return JNI_FALSE;
    }

    const char *name_dot = get_class_name(env, clazz);
    char* name = _strdup(name_dot);
    for (char* p = name; *p; ++p) if (*p == '.') *p = '/';

    jbyte* bytes = env->GetByteArrayElements(classBytes, NULL);
    jint len = env->GetArrayLength(classBytes);

    if (RetransformClassCache.size >= RetransformClassCache.capacity) {
        int new_capacity = RetransformClassCache.capacity == 0 ? 4 : RetransformClassCache.capacity * 2;
        RetransformClassCache.arr = (ClassBytesCacheType*)realloc(RetransformClassCache.arr, new_capacity * sizeof(ClassBytesCacheType));
        RetransformClassCache.capacity = new_capacity;
    }

    ClassBytesCacheType *entry = &RetransformClassCache.arr[RetransformClassCache.size++];
    entry->name = _strdup(name);
    entry->bytes = (unsigned char*)malloc(len);
    memcpy(entry->bytes, bytes, len);
    entry->len = len;

    env->ReleaseByteArrayElements(classBytes, bytes, JNI_ABORT);

    jclass classes[1] = { clazz };
    jvmtiError err = JuiceLoaderNative.jvmti->RetransformClasses(1, classes);
    if (err != JVMTI_ERROR_NONE) {
        PLOGE.printf("RetransformClasses failed! [err=%d]", err);
        free(name);
        return JNI_FALSE;
    }

    PLOGI.printf("[retransformClass] Class retransformed: %s (len=%d)", entry->name, entry->len);
    free(name);
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceloader_JuiceLoader_retransformClassByName
(JNIEnv *env, jclass loader_class, jstring className, jbyteArray classBytes, jint length) {
    const char *name_dot = env->GetStringUTFChars(className, NULL);
    char *name = _strdup(name_dot);
    for (char *p = name; *p; ++p) if (*p == '.') *p = '/';
    env->ReleaseStringUTFChars(className, name_dot);

    jclass target = env->FindClass(name);
    free(name);

    if (!target) {
        PLOGE << "FindClass failed in retransformClassByName";
        return JNI_FALSE;
    }

    return Java_cn_xiaozhou233_juiceloader_JuiceLoader_retransformClass(env, loader_class, target, classBytes, length);
}