/*
Author: xiaozhou233 (xiaozhou233@vip.qq.com)
File: juiceloader.c 
Desc: JuiceLoader Native, Provide jni function for JuiceLoader
*/

#include <windows.h>
#include "log.h"
#include "juiceloader.h"
#include "javautils.h"

JuiceLoaderNativeType JuiceLoaderNative;

static const char* g_bytecodes_classname = NULL;
static unsigned char* g_bytecodes = NULL;
static jint g_bytecodes_len = 0;

static void initLogger() {
    log_set_level(LOG_TRACE);
    log_is_log_time(false);
    log_set_name("juiceloader");
    log_info("Logger initialized.");
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

    //log_trace("CallBack: ClassFileLoadHook [%s]", name);

    if (g_bytecodes_classname != NULL && g_bytecodes == NULL) {
        if (strcmp(name, g_bytecodes_classname) == 0) {
            g_bytecodes_len = class_data_len;
            g_bytecodes = (unsigned char*)malloc(class_data_len);
            memcpy(g_bytecodes, classbytes, class_data_len);

            *new_classbytes = NULL;
            *new_class_data_len = 0;
        }
    }
}


jint init_juiceloader(JNIEnv *env, jvmtiEnv *jvmti) {
    log_info("[*] libjuiceloader Version %d.%d Build %d", PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_BUILD_NUMBER);

    initLogger();
    if (env == NULL) {
        log_error("env is NULL! env=%p", env);
        return JNI_ERR;
    }
    if (jvmti == NULL) {
        log_error("jvmti is NULL! jvmti=%p", jvmti);
        return JNI_ERR;
    }
    JuiceLoaderNative.jvmti = jvmti;
    log_debug("[JuiceLoader] JuiceLoaderNative addr = %p", &JuiceLoaderNative);
    log_debug("[JuiceLoader] JVMTI ptr = %p", JuiceLoaderNative.jvmti);


    // Dynamic register jni functions
    jclass clazz = (*env)->FindClass(env, "cn/xiaozhou233/juiceloader/JuiceLoaderNative");
    if (clazz == NULL) {
        log_error("Cannot find JuiceLoaderNative class!");
        return JNI_ERR;
    }
    JNINativeMethod methods[] = {
        // JuiceLoaderNative.init()
        {"init", "()Z", (void *)&loader_init},
        // JuiceLoaderNative.injectJar(String(jarPath));
        {"injectJar", "(Ljava/lang/String;)Z", (void *)&loader_injectJar},
        // JuiceLoaderNative.redefineClass(Class<?> clazz, byte[] classBytes, int length);
        {"redefineClass", "(Ljava/lang/Class;[BI)Z", (void *)&loader_redefineClass_clazz},
        // JuiceLoaderNative.redefineClassByName(String className, byte[] classBytes, int length);
        {"redefineClassByName", "(Ljava/lang/String;[BI)Z", (void *)&loader_redefineClass_className},
        // JuiceLoaderNative.getLoadedClasses();
        {"getLoadedClasses", "()[Ljava/lang/Class;", (void *)&loader_getLoadedClasses},
        // public static native byte[] getClassBytes(Class<?> clazz);
        // public static native byte[] getClassBytesByName(String className);
        {"getClassBytes", "(Ljava/lang/Class;)[B", (void *)&loader_getClassBytes},
        { "getClassBytesByName", "(Ljava/lang/String;)[B", (void *)&loader_getClassBytesByName}
    };
    jint result = (*env)->RegisterNatives(env, clazz, methods, sizeof(methods) / sizeof(methods[0]));
    if (result != JNI_OK) {
        log_error("Cannot register JNI methods for JuiceLoaderNative class! [result=%d]", result);
        return JNI_ERR;
    }

    // Application ability
    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));
    caps.can_get_bytecodes = 1;
    caps.can_redefine_classes = 1;
    caps.can_redefine_any_class = 1;
    caps.can_retransform_classes = 1;
    caps.can_retransform_any_class = 1;    
    caps.can_generate_all_class_hook_events = 1;
    result = (*JuiceLoaderNative.jvmti)->AddCapabilities(JuiceLoaderNative.jvmti, &caps);
    if (result != JNI_OK) {
        log_error("Cannot add JVMTI capabilities! [result=%d]", result);
        return JNI_ERR;
    }

    // Register callbacks
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.ClassFileLoadHook = &ClassFileLoadHook;
    (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks));

    // Enable events
    (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, NULL);

    return JNI_OK;
}

/// ============ JNI Function ============ ///

// JuiceLoaderNative.init()
JNIEXPORT jboolean JNICALL loader_init(JNIEnv *env, jclass loader_class) {
    // Deprcated
    log_warn("JuiceLoaderNative.init() is deprecated, init will be done automatically.");
    return JNI_TRUE;
}

// JuiceLoaderNative.injectJar(String)
JNIEXPORT jboolean JNICALL loader_injectJar(JNIEnv *env, jclass loader_class, jstring jarPath) {
    log_info("injectJar Invoked!");
    jboolean return_value = JNI_FALSE;

    const char *pathStr = (*env)->GetStringUTFChars(env, jarPath, NULL);
    if (pathStr == NULL) {
        log_error("jar path is NULL!");
        return_value = JNI_FALSE;
        goto cleanup;
    }
    log_info("Injecting jar: %s", pathStr);

    if (JuiceLoaderNative.jvmti == NULL) {
        log_error("JuiceLoaderNative.jvmti is NULL! [jvmti=%p]", JuiceLoaderNative.jvmti);
        return_value = JNI_FALSE;
        goto cleanup;
    }
    
    jvmtiError result = (*JuiceLoaderNative.jvmti)->AddToSystemClassLoaderSearch(JuiceLoaderNative.jvmti, pathStr);
    if (result != JNI_OK) {
        log_error("Cannot inject jar [result=%d]", result);
        return_value = JNI_FALSE;
        goto cleanup;
    }

    return_value = JNI_TRUE;

cleanup:
    if (pathStr != NULL) 
        (*env)->ReleaseStringUTFChars(env, jarPath, pathStr);
    return return_value;
}

// JuiceLoaderNative.redefineClass(Class, byte[], int)
JNIEXPORT jboolean JNICALL loader_redefineClass_clazz(JNIEnv *env, jclass loader_class, jclass clazz, jbyteArray classBytes, jint length) {
    log_info("redefineClass_clazz Invoked!");
    jboolean return_value = JNI_FALSE;
    jbyte* buf = NULL;

    if (JuiceLoaderNative.jvmti == NULL) {
        log_error("JuiceLoaderNative.jvmti is NULL! [jvmti=%p]", JuiceLoaderNative.jvmti);
        return_value = JNI_FALSE;
        goto cleanup;
    }

    log_info("Creating class definition...");
    log_trace("class: %p", clazz);
    log_trace("Class bytes length: %d", length);
    buf = (*env)->GetByteArrayElements(env, classBytes, NULL);
    if (buf == NULL) {
        log_error("Cannot get class bytes");
        return_value = JNI_FALSE;
        goto cleanup;
    }
    jvmtiClassDefinition defs[1];
    defs[0].klass = clazz;
    defs[0].class_byte_count = length;
    defs[0].class_bytes = (const unsigned char*) buf;

    jvmtiError result = (*JuiceLoaderNative.jvmti)->RedefineClasses(JuiceLoaderNative.jvmti, 1, defs);
    if (result != JVMTI_ERROR_NONE) {
        log_error("Cannot redefine class [result=%d]", result);
        return_value = JNI_FALSE;
        goto cleanup;
    }
    return_value = JNI_TRUE;
    log_info("Class redefined successfully!");

cleanup:
    if (buf != NULL) {
        (*env)->ReleaseByteArrayElements(env, classBytes, buf, JNI_ABORT);
    }
    return return_value;
}

JNIEXPORT jboolean JNICALL loader_redefineClass_className(JNIEnv *env, jclass loader_class, jstring className, jbyteArray classBytes, jint length) {
    log_info("redefineClass_className Invoked!");
    jboolean return_value = JNI_FALSE;

    if (JuiceLoaderNative.jvmti == NULL) {
        log_error("JuiceLoaderNative.jvmti is NULL! [jvmti=%p]", JuiceLoaderNative.jvmti);
        return_value = JNI_FALSE;
        goto cleanup;
    }

    const char* cname = (*env)->GetStringUTFChars(env, className, NULL);
    log_info("Finding class %s", cname);
    if (cname == NULL) {
        log_error("Class name is NULL!");
        return_value = JNI_FALSE;
        goto cleanup;
    }

    jclass clazz = (*env)->FindClass(env, cname);
    if (clazz == NULL) {
        log_error("Class not found! [ClassName=%s, clazz=%p]", cname, clazz);
        return_value = JNI_FALSE;
        goto cleanup;
    }

    return_value = loader_redefineClass_clazz(env, loader_class, clazz, classBytes, length);

cleanup:
    if (cname != NULL) {
        (*env)->ReleaseStringUTFChars(env, className, cname);
    }

    return return_value;
}

JNIEXPORT jobjectArray JNICALL loader_getLoadedClasses(JNIEnv *env, jclass loader_class) {
    log_info("loader_getLoadedClasses invoked!");

    if (JuiceLoaderNative.jvmti == NULL) {
        log_error("JuiceLoaderNative.jvmti is NULL! [jvmti=%p]", JuiceLoaderNative.jvmti);
        return NULL;
    }

    // Debug
    jvmtiPhase phase; (*JuiceLoaderNative.jvmti)->GetPhase(JuiceLoaderNative.jvmti, &phase); 
    log_debug("JVMTI Phase=%d [getLoadedClasses]", phase);

    jint count = 0;
    jclass* classes = NULL;

    jvmtiError result = (*JuiceLoaderNative.jvmti)->GetLoadedClasses(JuiceLoaderNative.jvmti, &count, &classes);
    if (result != JVMTI_ERROR_NONE || count == 0) {
        (*JuiceLoaderNative.jvmti)->Deallocate(JuiceLoaderNative.jvmti, (unsigned char*)classes);
        return (*env)->NewObjectArray(env, 0, (*env)->FindClass(env, "java/lang/Class"), NULL);
    }
    
    jclass classClass = (*env)->FindClass(env, "java/lang/Class");
    jobjectArray classArray = (*env)->NewObjectArray(env, count, classClass, NULL);

    for (int i = 0; i < count; i++) {
        (*env)->SetObjectArrayElement(env, classArray, i, classes[i]);
    }

    if (classes != NULL) {
        (*JuiceLoaderNative.jvmti)->Deallocate(JuiceLoaderNative.jvmti, (unsigned char*)classes);
    }
    return classArray;
}

JNIEXPORT jbyteArray JNICALL loader_getClassBytes(JNIEnv *env, jclass loader_class, jclass clazz) {
    log_info("getClassBytes Invoked!");

    if (JuiceLoaderNative.jvmti == NULL) {
        log_error("JuiceLoaderNative.jvmti is NULL! [jvmti=%p]", JuiceLoaderNative.jvmti);
        return NULL;
    }

    char* className = get_class_name(env, clazz);
    if (className == NULL) {
        log_error("Failed to get class name!");
        return NULL;
    }

    // JVM 内部使用路径名（. 替换为 /）
    for (char* p = className; *p; p++) {
        if (*p == '.') *p = '/';
    }

    g_bytecodes = NULL;
    g_bytecodes_classname = className;
    g_bytecodes_len = 0;

    jclass classes[1] = { clazz };
    jvmtiError err = (*JuiceLoaderNative.jvmti)->RetransformClasses(JuiceLoaderNative.jvmti, 1, classes);
    if (err != JVMTI_ERROR_NONE) {
        log_error("RetransformClasses failed: %d", err);
        free((void*)className);
        return NULL;
    }

    if (g_bytecodes != NULL) {
        jbyteArray result = (*env)->NewByteArray(env, g_bytecodes_len);
        (*env)->SetByteArrayRegion(env, result, 0, g_bytecodes_len, (jbyte*)g_bytecodes);

        free((void*)g_bytecodes);
        free((void*)className);
        g_bytecodes = NULL;
        g_bytecodes_classname = NULL;
        g_bytecodes_len = 0;

        return result;
    }

    log_warn("No bytecode captured for class: %s", className);
    free((void*)className);
    return NULL;
}

JNIEXPORT jbyteArray JNICALL loader_getClassBytesByName(JNIEnv *env, jclass loader_class, jstring className) {
    log_info("getClassBytesByName Invoked!");

    if (JuiceLoaderNative.jvmti == NULL) {
        log_error("JuiceLoaderNative.jvmti is NULL! [jvmti=%p]", JuiceLoaderNative.jvmti);
        return NULL;
    }

    const char* cname = (*env)->GetStringUTFChars(env, className, NULL);
    jclass clazz = (*env)->FindClass(env, cname);
    (*env)->ReleaseStringUTFChars(env, className, cname);

    if (clazz == NULL) {
        log_error("Failed to find class %s", cname);
        return NULL;
    }

    return loader_getClassBytes(env, loader_class, clazz);
}