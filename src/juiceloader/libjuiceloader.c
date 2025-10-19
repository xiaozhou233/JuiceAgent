#include "windows.h"
#include "JuiceLoaderNative.h"
#include "jvmti.h"
#include "log.h"
#include "JuiceLoaderUtils.h"
#include "JuiceLoader.h"

#define LOG_PREFIX "[JuiceLoader]"
static void check_jvmti_error(jvmtiEnv *j, jvmtiError err, const char *msg) {
    if (err != JVMTI_ERROR_NONE) {
        char *errname = NULL;
        (*j)->GetErrorName(j, err, &errname);
        log_error("JVMTI ERROR: %s: %d (%s)", msg, err, (errname==NULL?"Unknown":errname));
    }
}

/* Method entry callback */
static void JNICALL callbackMethodEntry(jvmtiEnv *j, JNIEnv *env, jthread thread, jmethodID method) {
    char *fullname = get_method_fullname(j, env, thread, method);
    if (fullname) {
        log_info("ENTER: %s", fullname);
        free(fullname);
    } else {
        log_warn("ENTER: <unknown method>");
    }
}


int setup(JNIEnv *env) {
    log_is_log_filename(false);
    log_is_log_line(false);
    log_is_log_time(false);
    log_set_level(LOG_DEBUG);

    log_info("[*] libjuiceloader Version %d.%d Build %d", PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_BUILD_NUMBER);

    log_info("%s setup!", LOG_PREFIX);
    {// Get JavaVM
    if (JNI_GetCreatedJavaVMs(&JuiceLoaderNative.jvm, 1, NULL) != JNI_OK || !JuiceLoaderNative.jvm) {
        log_error("%s JNI_GetCreatedJavaVMs failed", LOG_PREFIX);
        return 1;
    } else {
        log_info("%s JNI_GetCreatedJavaVMs success", LOG_PREFIX);
    }

    // Get JNIEnv
    if ((*JuiceLoaderNative.jvm)->AttachCurrentThread(JuiceLoaderNative.jvm, (void **)&JuiceLoaderNative.env, NULL) != JNI_OK) {
        log_error("%s AttachCurrentThread failed", LOG_PREFIX);
        return 1;
    } else {
        log_info("%s AttachCurrentThread success", LOG_PREFIX);
    }

    // Debug: JVM version
    jint version = (*JuiceLoaderNative.env)->GetVersion(JuiceLoaderNative.env);
    log_debug("%s JVM version: 0x%x", LOG_PREFIX, version);

    jint res = (*JuiceLoaderNative.jvm)->GetEnv(JuiceLoaderNative.jvm, (void**)&JuiceLoaderNative.jvmti, JVMTI_VERSION_1_2);
    if (res != JNI_OK || JuiceLoaderNative.jvmti == NULL) {
        log_error("%s GetEnv failed", LOG_PREFIX);
        return 1;
    } else {
        log_info("%s GetEnv success", LOG_PREFIX);
    }}
    log_info("%s Enabled JVMTI.");
    
    jvmtiError err;
//    jvmtiEventCallbacks callbacks;

    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));
    caps.can_get_bytecodes = 1;
    caps.can_redefine_classes = 1;
    caps.can_redefine_any_class = 1;
    caps.can_retransform_classes = 1;
    caps.can_retransform_any_class = 1;

    err = (*JuiceLoaderNative.jvmti)->AddCapabilities(JuiceLoaderNative.jvmti, &caps);
    check_jvmti_error(JuiceLoaderNative.jvmti, err, "AddCapabilities");

    // memset(&callbacks, 0, sizeof(callbacks));
    // callbacks.MethodEntry = &callbackMethodEntry;
    // err = (*JuiceLoaderNative.jvmti)->SetEventCallbacks(JuiceLoaderNative.jvmti, &callbacks, (jint)sizeof(callbacks));
    // check_jvmti_error(JuiceLoaderNative.jvmti, err, "SetEventCallbacks");

    // err = (*JuiceLoaderNative.jvmti)->SetEventNotificationMode(JuiceLoaderNative.jvmti, JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY, NULL);
    // check_jvmti_error(JuiceLoaderNative.jvmti, err, "EnableMethodEntry");

    return 0;
}

/*
    JNI Func: JuiceLoaderNative.init()
    Desc: Init environment
*/
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceloader_JuiceLoaderNative_init
  (JNIEnv *env, jobject obj) {
    log_info("%s JNI Func Init Invoke!");
    if(setup(env) == 0) {
        return JNI_TRUE;
    }
    return JNI_FALSE;
}

/*
    JNI Func: JuiceLoaderNative.injectJar(path)
    Desc: add jar to boostrap classloader
*/
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceloader_JuiceLoaderNative_injectJar
  (JNIEnv *env, jobject obj, jstring path) {
    log_info("%s JNI Func InjectJar Invoke!", LOG_PREFIX);
    jvmtiError err;
    if(JuiceLoaderNative.jvmti == NULL) {
        log_error("%s JVM not init!", LOG_PREFIX);
        return JNI_FALSE;
    }

    const char *pathStr = (*env)->GetStringUTFChars(env, path, NULL);
    log_info("%s InjectJar: %s", LOG_PREFIX, pathStr);

    err = (*JuiceLoaderNative.jvmti)->AddToBootstrapClassLoaderSearch(JuiceLoaderNative.jvmti, pathStr);
    check_jvmti_error(JuiceLoaderNative.jvmti, err, "AddToBootstrapClassLoaderSearch");
    return JNI_TRUE;
}

/*
    JNI Func: JuiceLoaderNative.redefine
    Desc: Redefine Class (class, bytecodes, len)
*/
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceloader_JuiceLoaderNative_redefineClass__Ljava_lang_Class_2_3BI
  (JNIEnv *env, jobject obj, jclass clazz, jbyteArray bytecodes, jint len) {
    log_info("%s JNI Func RedefineClass Invoke!", LOG_PREFIX);
    jvmtiError err;

    if (JuiceLoaderNative.jvmti == NULL) {
        log_error("%s JVM not init!", LOG_PREFIX);
        return JNI_FALSE;
    }

    log_info("%s Creating class definition...", LOG_PREFIX);
    jbyte* buf = (*env)->GetByteArrayElements(env, bytecodes, NULL);  // get raw bytes

    jvmtiClassDefinition defs[1];
    defs[0].klass = clazz;
    defs[0].class_byte_count = len;
    defs[0].class_bytes = (const unsigned char*) buf;

    log_info("%s Redefining class...", LOG_PREFIX);
    err = (*JuiceLoaderNative.jvmti)->RedefineClasses(JuiceLoaderNative.jvmti, 1, defs);
    check_jvmti_error(JuiceLoaderNative.jvmti, err, "RedefineClasses");

    (*env)->ReleaseByteArrayElements(env, bytecodes, buf, JNI_ABORT); // do not copy back, just release

    log_info("%s RedefineClass Done!", LOG_PREFIX);
    return JNI_TRUE;
}


/*
    JNI Func: JuiceLoaderNative.redifine
    Desc: Redefine Class (classname, bytecodes, len)
*/
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceloader_JuiceLoaderNative_redefineClass__Ljava_lang_String_2_3BI
  (JNIEnv *env, jobject obj, jstring classname, jbyteArray bytecodes, jint len) {
    log_info("%s RedefineClass Invoke!", LOG_PREFIX);
    jvmtiError err;

    if (JuiceLoaderNative.jvmti == NULL) {
        log_error("%s JVM not init!", LOG_PREFIX);
        return JNI_FALSE;
    }

    // --- 获取 class 名称 ---
    const char* cname = (*env)->GetStringUTFChars(env, classname, NULL);
    log_debug("%s Finding class: %s", LOG_PREFIX, cname);

    jclass clazz = (*env)->FindClass(env, cname);
    (*env)->ReleaseStringUTFChars(env, classname, cname);  // 正确释放

    if (clazz == NULL) {
        log_error("%s Class not found!", LOG_PREFIX);
        return JNI_FALSE;
    }

    // --- 获取字节数组 ---
    jbyte* buf = (*env)->GetByteArrayElements(env, bytecodes, NULL);

    jvmtiClassDefinition defs[1];
    defs[0].klass = clazz;
    defs[0].class_byte_count = len;
    defs[0].class_bytes = (const unsigned char*) buf;

    log_info("%s Redefining class...", LOG_PREFIX);
    err = (*JuiceLoaderNative.jvmti)->RedefineClasses(JuiceLoaderNative.jvmti, 1, defs);
    check_jvmti_error(JuiceLoaderNative.jvmti, err, "RedefineClasses");

    (*env)->ReleaseByteArrayElements(env, bytecodes, buf, JNI_ABORT);  // 用之前的 buf

    log_info("%s Done!", LOG_PREFIX);
    return JNI_TRUE;
}

JNIEXPORT jobjectArray JNICALL Java_cn_xiaozhou233_juiceloader_JuiceLoaderNative_getLoadedClasses
  (JNIEnv *env, jobject obj) {
        jint count = 0;
    jclass* classes = NULL;

    jvmtiError err = (*JuiceLoaderNative.jvmti)->GetLoadedClasses(JuiceLoaderNative.jvmti, &count, &classes);
    if (err != JVMTI_ERROR_NONE || count == 0) {
        return (*env)->NewObjectArray(env, 0, (*env)->FindClass(env, "java/lang/Class"), NULL);
    }

    jclass classClass = (*env)->FindClass(env, "java/lang/Class");
    jobjectArray result = (*env)->NewObjectArray(env, count, classClass, NULL);

    for (int i = 0; i < count; i++) {
        (*env)->SetObjectArrayElement(env, result, i, classes[i]);
    }

    (*JuiceLoaderNative.jvmti)->Deallocate(JuiceLoaderNative.jvmti, (unsigned char*)classes);
    return result;
  }