#include <juiceloader.h>
#include <jni.h>
#include <jvmti.h>
#include <string.h>
#include <stdlib.h>
#include <Logger.hpp>

RetransformClassCacheType RetransformClassCache = {NULL, 0, 0};

static const char* g_bytecodes_classname = NULL;
static unsigned char* g_bytecodes = NULL;
static jint g_bytecodes_len = 0;

/// ================= Utility ================= ///

char* get_class_name(JNIEnv* env, jclass clazz) {
    jclass clsClass = env->FindClass("java/lang/Class");
    jmethodID mid_getName = env->GetMethodID(clsClass, "getName", "()Ljava/lang/String;");
    jstring nameStr = (jstring)env->CallObjectMethod(clazz, mid_getName);

    const char* temp = env->GetStringUTFChars(nameStr, NULL);
    char* result = _strdup(temp);
    env->ReleaseStringUTFChars(nameStr, temp);
    env->DeleteLocalRef(nameStr);
    return result;
}

/// ================= ClassFileLoadHook ================= ///

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


/// ============ JNI Function ============ ///
JNIEXPORT jboolean JNICALL loader_init(JNIEnv *env, jclass loader_class) {
    PLOGW << "JuiceLoaderNative.init() is deprecated, init will be done automatically.";
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL loader_injectJar(JNIEnv *env, jclass loader_class, jstring jarPath) {
    PLOGI << "Invoked!";
    const char *pathStr = env->GetStringUTFChars(jarPath, NULL);
    if (!pathStr) {
        PLOGE << "Path is NULL!";
        return JNI_FALSE;
    }

    if (!JuiceLoaderNative.jvmti) {
        PLOGE.printf("JuiceLoaderNative.jvmti is NULL! [jvmti=%p]", JuiceLoaderNative.jvmti);
        env->ReleaseStringUTFChars(jarPath, pathStr);
        return JNI_FALSE;
    }

    jvmtiError result = JuiceLoaderNative.jvmti->AddToSystemClassLoaderSearch(pathStr);
    if (result != JVMTI_ERROR_NONE) {
        PLOGE.printf("Cannot inject jar [result=%d]", result);
        env->ReleaseStringUTFChars(jarPath, pathStr);
        return JNI_FALSE;
    }

    env->ReleaseStringUTFChars(jarPath, pathStr);
    PLOGI << "Jar injected: " << pathStr;
    return JNI_TRUE;
}
JNIEXPORT jboolean JNICALL loader_redefineClass_clazz(JNIEnv *env, jclass loader_class, jclass clazz, jbyteArray classBytes, jint length) {
    PLOGI << "Invoked!";
    if (!JuiceLoaderNative.jvmti) {
        PLOGE.printf("JuiceLoaderNative.jvmti is NULL! [jvmti=%p]", JuiceLoaderNative.jvmti);
        return JNI_FALSE;
    }

    jbyte* buf = env->GetByteArrayElements(classBytes, NULL);
    if (!buf) {
        PLOGE << "Cannot get class bytes";
        return JNI_FALSE;
    }

    jvmtiClassDefinition defs[1];
    defs[0].klass = clazz;
    defs[0].class_byte_count = length;
    defs[0].class_bytes = reinterpret_cast<const unsigned char*>(buf);

    jvmtiError result = JuiceLoaderNative.jvmti->RedefineClasses(1, defs);
    env->ReleaseByteArrayElements(classBytes, buf, JNI_ABORT);

    if (result != JVMTI_ERROR_NONE) {
        PLOGE.printf("Cannot redefine class [result=%d]", result);
        return JNI_FALSE;
    }

    PLOGI.printf("Class redefined successfully!");
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL loader_redefineClass_className(JNIEnv *env, jclass loader_class, jstring className, jbyteArray classBytes, jint length) {
    PLOGI << "redefineClass_className Invoked!";
    if (!JuiceLoaderNative.jvmti) {
        PLOGE.printf("JuiceLoaderNative.jvmti is NULL! [jvmti=%p]", JuiceLoaderNative.jvmti);
        return JNI_FALSE;
    }

    const char* cname = env->GetStringUTFChars(className, NULL);
    if (!cname) {
        PLOGE << "Class name is NULL!";
        return JNI_FALSE;
    }

    jclass clazz = env->FindClass(cname);
    if (!clazz) {
        PLOGE.printf("Class not found! [ClassName=%s]", cname);
        env->ReleaseStringUTFChars(className, cname);
        return JNI_FALSE;
    }

    jboolean ret = loader_redefineClass_clazz(env, loader_class, clazz, classBytes, length);
    env->ReleaseStringUTFChars(className, cname);
    return ret;
}

JNIEXPORT jobjectArray JNICALL loader_getLoadedClasses(JNIEnv *env, jclass loader_class) {
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

JNIEXPORT jbyteArray JNICALL loader_getClassBytes(JNIEnv *env, jclass loader_class, jclass clazz) {
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

JNIEXPORT jbyteArray JNICALL loader_getClassBytesByName(JNIEnv *env, jclass loader_class, jstring className) {
    PLOGI << "getClassBytesByName Invoked!";
    if (!JuiceLoaderNative.jvmti) {
        PLOGE << "JuiceLoaderNative.jvmti is NULL!";
        return NULL;
    }

    const char* cname = env->GetStringUTFChars(className, NULL);
    jclass clazz = env->FindClass(cname);
    env->ReleaseStringUTFChars(className, cname);

    if (!clazz) {
        PLOGE.printf("Failed to find class %s", cname);
        return NULL;
    }

    return loader_getClassBytes(env, loader_class, clazz);
}
JNIEXPORT jboolean JNICALL loader_retransformClass(JNIEnv *env, jclass loader_class, jclass clazz, jbyteArray classBytes, jint length) {
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

JNIEXPORT jboolean JNICALL loader_retransformClassByName(JNIEnv *env, jclass loader_class, jstring className, jbyteArray classBytes, jint length) {
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

    return loader_retransformClass(env, loader_class, target, classBytes, length);
}

JNIEXPORT jclass JNICALL loader_getClassByName(JNIEnv *env, jclass loader_class, jstring className) {
    
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