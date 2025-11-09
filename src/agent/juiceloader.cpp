// JVM
#include <jni.h>
#include <jvmti.h>
#include <Logger.hpp>
#include <juiceloader.h>

JuiceLoaderNativeType JuiceLoaderNative;

static int RegisterNatives(JNIEnv *env) {
    const char* native_class = "cn/xiaozhou233/juiceloader/JuiceLoader";
    JNINativeMethod methods[] = {
        {(char*)"init", (char*)"()Z", (void *)&loader_init},
        {(char*)"injectJar", (char*)"(Ljava/lang/String;)Z", (void *)&loader_injectJar},
        {(char*)"redefineClass", (char*)"(Ljava/lang/Class;[BI)Z", (void *)&loader_redefineClass_clazz},
        {(char*)"redefineClassByName", (char*)"(Ljava/lang/String;[BI)Z", (void *)&loader_redefineClass_className},
        {(char*)"getLoadedClasses", (char*)"()[Ljava/lang/Class;", (void *)&loader_getLoadedClasses},
        {(char*)"getClassBytes", (char*)"(Ljava/lang/Class;)[B", (void *)&loader_getClassBytes},
        {(char*)"getClassBytesByName", (char*)"(Ljava/lang/String;)[B", (void *)&loader_getClassBytesByName},
        {(char*)"retransformClass", (char*)"(Ljava/lang/Class;[BI)Z", (void *)&loader_retransformClass},
        {(char*)"retransformClassByName", (char*)"(Ljava/lang/String;[BI)Z", (void *)&loader_retransformClassByName},
        {(char*)"getClassByName", (char*)"(Ljava/lang/String;)Ljava/lang/Class;", (void *)&loader_getClassByName},
        // public static native Thread nativeGetThreadByName(String var0);
        {(char*)"nativeGetThreadByName", (char*)"(Ljava/lang/String;)Ljava/lang/Thread;", (void *)&loader_nativeGetThreadByName},
        // public static native ClassLoader nativeInjectJarToThread(Thread var0, String var1);
        {(char*)"nativeInjectJarToThread", (char*)"(Ljava/lang/Thread;Ljava/lang/String;)Ljava/lang/ClassLoader;", (void *)&loader_nativeInjectJarToThread}
    };

    // Find Native(JNI) Class
    jclass clazz = env->FindClass("cn/xiaozhou233/juiceloader/JuiceLoader");
    if (clazz == NULL) {
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        PLOGE.printf("[-] Failed to initialize libjuiceloader: JuiceLoader class not found");
        return -1;
    }

    // Dynamicly register native methods
    jint result = env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0]));
    if (result != JNI_OK) {
        PLOGE << "Failed to register native methods: " << result;
        return JNI_ERR;
    }

    return JNI_OK;
}

static int ConfigureJVMTI(JNIEnv *env, _jvmtiEnv *jvmti) {
    // Retransform Class Cache : to store classes that need to be retransformed
    RetransformClassCache.arr = NULL;
    RetransformClassCache.size = 0;
    RetransformClassCache.capacity = 0;

    // Application ability
    jvmtiCapabilities caps;
    memset(&caps, 0, sizeof(caps));
    caps.can_get_bytecodes = 1;
    caps.can_redefine_classes = 1;
    caps.can_redefine_any_class = 1;
    caps.can_retransform_classes = 1;
    caps.can_retransform_any_class = 1;    
    caps.can_generate_all_class_hook_events = 1;
    jint result = jvmti->AddCapabilities(&caps);
    if (result != JNI_OK) {
        PLOGE << "Failed to add capabilities: " << result;
        return JNI_ERR;
    }

    // Register callbacks
    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.ClassFileLoadHook = &ClassFileLoadHook;
    jvmti->SetEventCallbacks(&callbacks, sizeof(callbacks));

    // Enable events
    jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, NULL);

    return JNI_OK;
}

int InitJuiceLoader(JNIEnv *env, _jvmtiEnv *jvmti) {
    PLOGI.printf("[*] libjuiceloader Version %d.%d Build %d", PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_BUILD_NUMBER);
    // Step 1: Check if env/jvmti is null
    PLOGD.printf("env: %p, jvmti: %p", env, jvmti);
    if (env == nullptr) {
        PLOGE.printf("[-] Failed to initialize libjuiceloader: env is null");
        return -1;
    }
    if (jvmti == nullptr) {
        PLOGE.printf("[-] Failed to initialize libjuiceloader: jvmti is null");
        return -1;
    }
    JuiceLoaderNative.jvmti = jvmti;

    // Step 2: Register Native Methods
    if (RegisterNatives(env) != JNI_OK) {
        PLOGE.printf("[-] Failed to initialize libjuiceloader: RegisterNatives failed");
        return -1;
    }

    // Step 3: Configure JVMTI
    if (ConfigureJVMTI(env, jvmti) != JNI_OK) {
        PLOGE.printf("[-] Failed to initialize libjuiceloader: ConfigureJVMTI failed");
        return -1;
    }

    return 0;
}


