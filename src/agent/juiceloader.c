/*
Author: xiaozhou233 (xiaozhou233@vip.qq.com)
File: juiceloader.c 
Desc: JuiceLoader Native, Provide jni function for JuiceLoader
*/

#include <windows.h>
#include "log.h"
#include "juiceloader.h"

JuiceLoaderNativeType JuiceLoaderNative;

static void initLogger() {
    log_set_level(LOG_TRACE);
    log_is_log_time(false);
    log_set_name("juiceloader");
    log_info("Logger initialized.");
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
        {"getLoadedClasses", "()[Ljava/lang/Class;", (void *)&loader_getLoadedClasses}
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
    result = (*JuiceLoaderNative.jvmti)->AddCapabilities(JuiceLoaderNative.jvmti, &caps);
    if (result != JNI_OK) {
        log_error("Cannot add JVMTI capabilities! [result=%d]", result);
        return JNI_ERR;
    }

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
/// ============ JNI Function ============ ///