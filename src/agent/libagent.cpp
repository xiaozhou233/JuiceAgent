#include <libagent.h>
#include <JuiceAgent/Logger.hpp>
#include <jni.h>
#include <jvmti.h>
#include <windows.h>

struct _JuiceAgent {
    bool isAttached;
    JavaVM *jvm;
    jvmtiEnv *jvmti;
    JNIEnv *env;
} JuiceAgent = {false, NULL, NULL, NULL};

static int GetJavaEnv() {
    jint res = JNI_ERR;
    JavaVM *jvm = NULL;
    jvmtiEnv *jvmti = NULL;
    JNIEnv *env = NULL;
    int max_try = 30;

    // Get the JVM
    max_try = 30;
    do {
        res = JNI_GetCreatedJavaVMs(&jvm, 1, NULL);
        if (res != JNI_OK || jvm == NULL) {
            PLOGE.printf("JNI_GetCreatedJavaVMs failed: %d", res);
            PLOGD << "JVM NULL: " << (jvm == NULL);
        }
        Sleep(1000);
        max_try--;
        if (jvm != NULL) {
            break;
        } else {
            PLOGI.printf("Waiting for JVM... (%d)", max_try);
        }
    } while (jvm == NULL && max_try > 0);
    if (jvm == NULL) {
        PLOGE << "GetJavaEnv failed";
        return res;
    }
    JuiceAgent.jvm = jvm;
    PLOGI << "Got the JVM";

    // Get the JNIEnv
    max_try = 30;
    do {
        res = jvm->GetEnv((void**)&env, JNI_VERSION_1_8);
        if (res != JNI_OK || env == NULL) {
            PLOGI << "GetEnv failed, trying to attach.";
            PLOGD << "env NULL: " << (env == NULL);
            res = jvm->AttachCurrentThread((void**)&env, NULL);
            if (res != JNI_OK || env == NULL) {
                PLOGE.printf("AttachCurrentThread failed: %d", res);
                PLOGD << "env NULL: " << (env == NULL);
                return res;
            }
        }
        Sleep(1000);
        max_try--;
        if (env != NULL) {
            break;
        } else {
            PLOGI.printf("Waiting for JNI... (%d)", max_try);
        }
    } while (env == NULL && max_try > 0);
    if (env == NULL) {
        PLOGE << "Get JNI failed";
        return res;
    }
    JuiceAgent.env = env;
    PLOGI << "Got the JNIEnv";
    
    // Get the JVMTI
    res = jvm->GetEnv((void**)&jvmti, JVMTI_VERSION_1_2);
    if (res != JNI_OK || jvmti == NULL) {
        PLOGE.printf("GetEnv failed: %d", res);
        PLOGD << "jvmti NULL: " << (jvmti == NULL);
        return res;
    }
    JuiceAgent.jvmti = jvmti;
    PLOGI << "Got the JVMTI";

    return 0;
}

static int InitLoader() {
    jint res = JNI_ERR;
    JNIEnv *env = JuiceAgent.env;
    jvmtiEnv *jvmti = JuiceAgent.jvmti;
    const char *bootstrap_class = "cn/xiaozhou233/juiceloader/JuiceLoaderBootstrap";
    const char *method_name = "start";
    const char *method_signature = "()V";


    PLOGI << "Loading JuiceLoader Jar...";
    // TODO: Load JuiceLoader Jar
    PLOGI << "JuiceLoader Jar [OK]";

    PLOGI << "Invoke JuiceLoader Bootstrap...";
    // Find class
    jclass cls = env->FindClass(bootstrap_class);
    if (cls == NULL) {
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        PLOGE << "FindClass failed";
        return 1;
    }

    // Find method
    jmethodID mid = env->GetStaticMethodID(cls, method_name, method_signature);
    if (mid == NULL) {
        PLOGE << "GetStaticMethodID failed";
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        return 1;
    }
    // Invoke method
    env->CallStaticVoidMethod(cls, mid);
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();

        PLOGE << "CallStaticVoidMethod failed";
        return 1;
    }

    PLOGI << "JuiceLoader Bootstrap [OK]";
    return 0;
}
void EntryPoint() {
    PLOGI << "EnrtryPoint Invoked!";

    PLOGI << "Trying to Get Java Environment...";
    if (GetJavaEnv() != JNI_OK) {
        PLOGE << "Java Environment [Failed]";
        return;
    }
    PLOGI << "Java Environment [OK]";

    PLOGI << "Trying to Init JuiceLoader...";
    if (InitLoader() != 0) {
        PLOGE << "JuiceLoader [Failed]";
        return;
    }
    PLOGI << "JuiceLoader [OK]";

    PLOGI << "JuiceAgent Bootstrap [OK]";
    PLOGI << "Init going to JuiceLoader now...";
    JuiceAgent.isAttached = true;
}