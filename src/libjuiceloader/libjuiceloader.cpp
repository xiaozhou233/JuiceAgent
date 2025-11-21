// JVM
#include <jni.h>
#include <jvmti.h>
#include <JuiceAgent/Logger.hpp>
#include <juiceloader.h>

JuiceLoaderNativeType JuiceLoaderNative;

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
    // callbacks.ClassFileLoadHook = &ClassFileLoadHook;
    jvmti->SetEventCallbacks(&callbacks, sizeof(callbacks));

    // Enable events
    jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, NULL);

    return JNI_OK;
}

extern "C" __declspec(dllexport)
int InitJuiceLoader(JNIEnv *env, _jvmtiEnv *jvmti) {
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


    // Step 2: Configure JVMTI
    if (ConfigureJVMTI(env, jvmti) != JNI_OK) {
        PLOGE.printf("[-] Failed to initialize libjuiceloader: ConfigureJVMTI failed");
        return -1;
    }

    return 0;
}

