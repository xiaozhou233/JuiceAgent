#include <jni_impl.h>

const char* g_bytecodes_classname = NULL;
unsigned char* g_bytecodes = NULL;
jint g_bytecodes_len = 0;

JNIEXPORT jobjectArray JNICALL Java_cn_xiaozhou233_juiceloader_JuiceLoader_getLoadedClasses
(JNIEnv *env, jclass loader_class) {
    PLOGI << "loader_getLoadedClasses invoked!";

    if (!JuiceLoaderNative.jvmti) {
        PLOGE << "JuiceLoaderNative.jvmti is NULL!";
        return NULL;
    }

    jint count = 0;
    jclass* classes = NULL;
    jvmtiError result = JuiceLoaderNative.jvmti->GetLoadedClasses(&count, &classes);
    if (result != JVMTI_ERROR_NONE || count == 0) {
        if (classes) JuiceLoaderNative.jvmti->Deallocate((unsigned char*)classes);
        return env->NewObjectArray(0, env->FindClass("java/lang/Class"), NULL);
    }

    jclass classClass = env->FindClass("java/lang/Class");
    jobjectArray classArray = env->NewObjectArray(count, classClass, NULL);

    for (int i = 0; i < count; i++) {
        env->SetObjectArrayElement(classArray, i, classes[i]);
    }

    if (classes) JuiceLoaderNative.jvmti->Deallocate((unsigned char*)classes);
    return classArray;
}

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

static jclass findLoadedClassByInternalName(
        jvmtiEnv* jvmti,
        JNIEnv* env,
        const char* internalName
) {
    jint count = 0;
    jclass* classes = nullptr;

    if (jvmti->GetLoadedClasses(&count, &classes) != JVMTI_ERROR_NONE) {
        return nullptr;
    }

    jclass result = nullptr;

    for (jint i = 0; i < count; i++) {
        char* signature = nullptr;

        if (jvmti->GetClassSignature(classes[i], &signature, nullptr)
            == JVMTI_ERROR_NONE) {

            // signature example: Ljava/lang/String;
            if (signature[0] == 'L') {
                size_t len = strlen(signature);
                if (len > 2) {
                    // strip 'L' and ';'
                    if (strncmp(signature + 1, internalName, len - 2) == 0 &&
                        internalName[len - 2] == '\0') {
                        result = classes[i];
                        jvmti->Deallocate((unsigned char*)signature);
                        break;
                    }
                }
            }

            jvmti->Deallocate((unsigned char*)signature);
        }
    }

    jvmti->Deallocate((unsigned char*)classes);
    return result;
}


JNIEXPORT jbyteArray JNICALL Java_cn_xiaozhou233_juiceloader_JuiceLoader_getClassBytes
(JNIEnv *env, jclass loader_class, jclass clazz) {
    PLOGI << "getClassBytes Invoked!";
    if (!JuiceLoaderNative.jvmti) {
        PLOGE << "JuiceLoaderNative.jvmti is NULL!";
        return NULL;
    }

    char* className = get_class_name(env, clazz);
    if (!className) {
        PLOGE << "Failed to get class name!";
        return NULL;
    }

    for (char* p = className; *p; ++p) if (*p == '.') *p = '/';

    g_bytecodes_classname = className;
    g_bytecodes = NULL;
    g_bytecodes_len = 0;

    jclass classes[1] = { clazz };
    jvmtiError err = JuiceLoaderNative.jvmti->RetransformClasses(1, classes);
    if (err != JVMTI_ERROR_NONE) {
        PLOGE.printf("RetransformClasses failed: %d", err);
        free(className);
        return NULL;
    }

    if (!g_bytecodes) {
        PLOGW.printf("No bytecode captured for class: %s", className);
        free(className);
        return NULL;
    }

    jbyteArray result = env->NewByteArray(g_bytecodes_len);
    env->SetByteArrayRegion(result, 0, g_bytecodes_len, (jbyte*)g_bytecodes);

    free(g_bytecodes);
    free(className);
    g_bytecodes = NULL;
    g_bytecodes_classname = NULL;
    g_bytecodes_len = 0;

    return result;
}

JNIEXPORT jbyteArray JNICALL
Java_cn_xiaozhou233_juiceloader_JuiceLoader_getClassBytesByName
(JNIEnv* env, jclass loader_class, jstring className) {

    PLOGI << "getClassBytesByName (JVMTI enumerate) Invoked";

    if (!JuiceLoaderNative.jvmti || className == nullptr) {
        PLOGE << "JVMTI or className is NULL";
        return nullptr;
    }

    // jstring -> UTF-8
    const char* nameUtf = env->GetStringUTFChars(className, nullptr);
    if (!nameUtf) {
        return nullptr;
    }

    // normalize: dot -> slash
    char internalName[512];
    strncpy(internalName, nameUtf, sizeof(internalName) - 1);
    internalName[sizeof(internalName) - 1] = '\0';

    for (char* p = internalName; *p; ++p) {
        if (*p == '.') *p = '/';
    }

    env->ReleaseStringUTFChars(className, nameUtf);

    // find class via JVMTI
    jclass targetClass =
        findLoadedClassByInternalName(JuiceLoaderNative.jvmti, env, internalName);

    if (!targetClass) {
        PLOGE.printf("Class not loaded: %s", internalName);
        return nullptr;
    }

    // reuse existing getClassBytes(Class)
    return Java_cn_xiaozhou233_juiceloader_JuiceLoader_getClassBytes(
        env, loader_class, targetClass
    );
}


JNIEXPORT jclass JNICALL Java_cn_xiaozhou233_juiceloader_JuiceLoader_getClassByName
(JNIEnv *env, jclass loader_class, jstring className) {
    
    if (className == NULL) {
        return NULL;
    }

    // jstring -> const char*
    const char *name_utf = env->GetStringUTFChars(className, NULL);
    if (name_utf == NULL) {
        return NULL;
    }

    // find class
    jclass clazz = env->FindClass(name_utf);

    // clean up
    env->ReleaseStringUTFChars(className, name_utf);

    if (clazz == NULL) {
        PLOGE.printf("Failed to find class %s", name_utf);
    }
    return clazz;
}