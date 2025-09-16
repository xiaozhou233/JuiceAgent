#include "windows.h"
#include "JuiceLoaderNative.h"
#include "jvmti.h"

struct _JuiceLoaderNative {
    JavaVM *jvm;
    jvmtiEnv *jvmti;
    JNIEnv *env;
} JuiceLoaderNative;

static void check_jvmti_error(jvmtiEnv *j, jvmtiError err, const char *msg) {
    if (err != JVMTI_ERROR_NONE) {
        char *errname = NULL;
        (*j)->GetErrorName(j, err, &errname);
        fprintf(stderr, "JVMTI ERROR: %s: %d (%s)\n", msg, err, (errname==NULL?"Unknown":errname));
    }
}

/* Helper: get method full name: ClassName.methodName(signature) */
static char* get_method_fullname(jvmtiEnv *j, JNIEnv *env, jthread thread, jmethodID method) {
    char *mname = NULL, *msig = NULL, *mgeneric = NULL;
    jclass declaring = NULL;
    char *cls_sig = NULL;
    char *result = NULL;

    jvmtiError err;

    err = (*j)->GetMethodName(j, method, &mname, &msig, &mgeneric);
    if (err != JVMTI_ERROR_NONE) goto cleanup;

    err = (*j)->GetMethodDeclaringClass(j, method, &declaring);
    if (err != JVMTI_ERROR_NONE) goto cleanup;

    err = (*j)->GetClassSignature(j, declaring, &cls_sig, NULL);
    if (err != JVMTI_ERROR_NONE) goto cleanup;

    /* cls_sig is like "Lcom/example/MyClass;" -> make it human-friendly */
    size_t len = strlen(cls_sig) + strlen(mname) + strlen(msig) + 4;
    result = (char*)malloc(len);
    if (result) {
        /* remove leading L and trailing ; from class signature if present */
        char *cls_nice = cls_sig;
        if (cls_sig[0] == 'L') cls_nice = cls_sig + 1;
        size_t cls_len = strlen(cls_nice);
        if (cls_nice[cls_len - 1] == ';') cls_nice[cls_len - 1] = '\0';
        snprintf(result, len, "%s.%s%s", cls_nice, mname, msig);
    }

cleanup:
    if (mname) (*j)->Deallocate(j, (unsigned char*)mname);
    if (msig) (*j)->Deallocate(j, (unsigned char*)msig);
    if (mgeneric) (*j)->Deallocate(j, (unsigned char*)mgeneric);
    if (cls_sig) (*j)->Deallocate(j, (unsigned char*)cls_sig);
    return result;
}

/* Method entry callback */
static void JNICALL callbackMethodEntry(jvmtiEnv *j, JNIEnv *env, jthread thread, jmethodID method) {
    char *fullname = get_method_fullname(j, env, thread, method);
    if (fullname) {
        printf("ENTER: %s\n", fullname);
        free(fullname);
    } else {
        printf("ENTER: <unknown method>\n");
    }
}


int setup(JNIEnv *env) {
    printf("setup!\n");
    {// Get JavaVM
    if (JNI_GetCreatedJavaVMs(&JuiceLoaderNative.jvm, 1, NULL) != JNI_OK || !JuiceLoaderNative.jvm) {
        printf("libjuiceloader: JNI_GetCreatedJavaVMs failed\n");
        return 1;
    } else {
        printf("libjuiceloader: JNI_GetCreatedJavaVMs success\n");
    }

    // Get JNIEnv
    if ((*JuiceLoaderNative.jvm)->AttachCurrentThread(JuiceLoaderNative.jvm, (void **)&JuiceLoaderNative.env, NULL) != JNI_OK) {
        printf("libjuiceloader: AttachCurrentThread failed\n");
        return 1;
    } else {
        printf("libjuiceloader: AttachCurrentThread success\n");
    }

    // Debug: JVM version
    jint version = (*JuiceLoaderNative.env)->GetVersion(JuiceLoaderNative.env);
    printf("JVM version: 0x%x\n", version);

    printf("libagent: enabling jvmti\n");
    jint res = (*JuiceLoaderNative.jvm)->GetEnv(JuiceLoaderNative.jvm, (void**)&JuiceLoaderNative.jvmti, JVMTI_VERSION_1_2);
    if (res != JNI_OK || JuiceLoaderNative.jvmti == NULL) {
        printf("libagent: GetEnv failed\n");
        return 1;
    } else {
        printf("libagent: GetEnv success\n");
    }}
    
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
    printf("JNI Func Init Invoke!\n");
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
    printf("JNI Func InjectJar Invoke!\n");
    jvmtiError err;
    if(JuiceLoaderNative.jvmti == NULL) {
        printf("JVM not init!\n");
        return JNI_FALSE;
    }

    const char *pathStr = (*env)->GetStringUTFChars(env, path, NULL);
    printf("InjectJar: %s\n", pathStr);

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
    printf("JNI Func RedefineClass Invoke!\n");
    jvmtiError err;

    if (JuiceLoaderNative.jvmti == NULL) {
        printf("JVM not init!\n");
        return JNI_FALSE;
    }

    printf("Creating class definition...\n");
    jbyte* buf = (*env)->GetByteArrayElements(env, bytecodes, NULL);  // get raw bytes

    jvmtiClassDefinition defs[1];
    defs[0].klass = clazz;
    defs[0].class_byte_count = len;
    defs[0].class_bytes = (const unsigned char*) buf;

    printf("Redefining class...\n");
    err = (*JuiceLoaderNative.jvmti)->RedefineClasses(JuiceLoaderNative.jvmti, 1, defs);
    check_jvmti_error(JuiceLoaderNative.jvmti, err, "RedefineClasses");

    (*env)->ReleaseByteArrayElements(env, bytecodes, buf, JNI_ABORT); // do not copy back, just release

    printf("Done!\n");
    return JNI_TRUE;
}


/*
    JNI Func: JuiceLoaderNative.redifine
    Desc: Redefine Class (classname, bytecodes, len)
*/
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceloader_JuiceLoaderNative_redefineClass__Ljava_lang_String_2_3BI
  (JNIEnv *env, jobject obj, jstring classname, jbyteArray bytecodes, jint len) {
    printf("JNI Func RedefineClass Invoke!\n");
    jvmtiError err;

    if (JuiceLoaderNative.jvmti == NULL) {
        printf("JVM not init!\n");
        return JNI_FALSE;
    }

    // --- 获取 class 名称 ---
    const char* cname = (*env)->GetStringUTFChars(env, classname, NULL);
    printf("Finding class: %s\n", cname);

    jclass clazz = (*env)->FindClass(env, cname);
    (*env)->ReleaseStringUTFChars(env, classname, cname);  // 正确释放

    if (clazz == NULL) {
        printf("Class not found!\n");
        return JNI_FALSE;
    }

    // --- 获取字节数组 ---
    jbyte* buf = (*env)->GetByteArrayElements(env, bytecodes, NULL);

    jvmtiClassDefinition defs[1];
    defs[0].klass = clazz;
    defs[0].class_byte_count = len;
    defs[0].class_bytes = (const unsigned char*) buf;

    printf("Redefining class...\n");
    err = (*JuiceLoaderNative.jvmti)->RedefineClasses(JuiceLoaderNative.jvmti, 1, defs);
    check_jvmti_error(JuiceLoaderNative.jvmti, err, "RedefineClasses");

    (*env)->ReleaseByteArrayElements(env, bytecodes, buf, JNI_ABORT);  // 用之前的 buf

    printf("Done!\n");
    return JNI_TRUE;
}
