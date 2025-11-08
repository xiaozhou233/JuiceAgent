// For Reflective Inject
#include "ReflectiveLoader.h"
#define WIN_X64

#include <windows.h>
#include "Logger.hpp"
#include <JuiceAgent.h>
#include <juiceloader.h>

// For Java VM
#include <jni.h>
#include <jvmti.h>

struct _JuiceAgent {
    bool isAttached;
    JavaVM *jvm;
    jvmtiEnv *jvmti;
    JNIEnv *env;
} JuiceAgent = {false, NULL, NULL, NULL};

HINSTANCE hAppInstance = nullptr;


static int GetJavaEnv() {
    jint res = JNI_ERR;
    JavaVM *jvm = NULL;
    jvmtiEnv *jvmti = NULL;
    JNIEnv *env = NULL;

    // Get the JVM
    res = JNI_GetCreatedJavaVMs(&jvm, 1, NULL);
    if (res != JNI_OK) {
        PLOGE.printf("JNI_GetCreatedJavaVMs failed: %d", res);
        return res;
    }
    JuiceAgent.jvm = jvm;
    PLOGI << "Got the JVM";

    // Get the JNIEnv
    res = jvm->GetEnv((void**)&env, JNI_VERSION_1_8);
    if (res != JNI_OK) {
        PLOGI << "GetEnv failed, trying to attach.";
        res = jvm->AttachCurrentThread((void**)&env, NULL);
        if (res != JNI_OK) {
            PLOGE.printf("AttachCurrentThread failed: %d", res);
            return res;
        }
    }
    JuiceAgent.env = env;
    PLOGI << "Got the JNIEnv";
    
    // Get the JVMTI
    res = jvm->GetEnv((void**)&jvmti, JVMTI_VERSION_1_2);
    if (res != JNI_OK) {
        PLOGE.printf("GetEnv failed: %d", res);
        return res;
    }
    JuiceAgent.jvmti = jvmti;
    PLOGI << "Got the JVMTI";

    return 0;
}

static int InvokeJuiceLoaderInit(const char* ConfigDir) {
    // Variables
    int result = JNI_ERR;
    JNIEnv *env = JuiceAgent.env;
    jvmtiEnv *jvmti = JuiceAgent.jvmti;

    const char *bootstrap_class = "cn/xiaozhou233/juiceloader/JuiceLoaderBootstrap";
    const char *method_name = "init";
    const char *method_signature = "(Ljava/lang/String;)V";

    PLOGI << "Invoke JuiceLoader Init";
    PLOGI << "\n\n================ JuiceLoader Init =================";
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
    env->CallStaticVoidMethod(cls, mid, env->NewStringUTF(ConfigDir));
    if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
    }

    PLOGI << "================ JuiceLoader Init =================\n\n";

    PLOGI << "Invoke JuiceLoader Init done";
    return 0;
}

int InitJuiceAgent(const char* ConfigDir, const char* JuiceLoaderJarPath) {
    // Variables
    int result = JNI_ERR;
    
    // Step 1: Get the Java environment
    if (GetJavaEnv() != 0) {
        PLOGE << "Failed to get Java environment code=" << result;
        return 1;
    }

    // Step 2: Inject JuiceLoader.jar
    result = JuiceAgent.jvmti->AddToSystemClassLoaderSearch(JuiceLoaderJarPath);
    if (result != JNI_OK) {
        PLOGE.printf("AddToSystemClassLoaderSearch failed: %d", result);
        return 1;
    }

    // Step 3: Invoke JuiceLoader Native Init
    if (InitJuiceLoader(JuiceAgent.env, JuiceAgent.jvmti) != JNI_OK) {
        PLOGE << "Failed to invoke JuiceLoader Native Init: " << result;
        return 1;
    } 

    // Step 4: Invoke JuiceLoader Init
    if (InvokeJuiceLoaderInit(ConfigDir) != 0) {
        PLOGE << "Failed to invoke JuiceLoader Init code=" << result;
        return 1;
    }

    // Step 5: Done, cleanup

    return 0;
}

DWORD WINAPI ThreadProc(LPVOID lpParam) {
    PLOGD << "ThreadProc";
    PLOGD.printf("Recieved lpParam: %p");
    if (lpParam == nullptr) {
        PLOGE << "lpParam is NULL";
        return 1;
    }

    InjectParams* pParams = (InjectParams*)lpParam;
    PLOGD << "ConfigDir: " << pParams->ConfigDir;
    PLOGD << "JuiceLoaderJarPath: " << pParams->JuiceLoaderJarPath;

    PLOGI << "Init JuiceAgent...";
    int result = InitJuiceAgent(pParams->ConfigDir, pParams->JuiceLoaderJarPath);
    PLOGI.printf("Init JuiceAgent result: %d", result);

    PLOGI << "Exit ThreadProc";
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH: {
            hAppInstance = hinstDLL;
            InitLogger();

            PLOGI << "DLL_PROCESS_ATTACH";
            PLOGI.printf("[*] JuiceAgent Version %d.%d Build %d", PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_BUILD_NUMBER);

            DisableThreadLibraryCalls(hinstDLL);

            // TODO: ReflectiveLoader Params
            InjectParams *testParams = (InjectParams*)malloc(sizeof(InjectParams));
            strcpy(testParams->ConfigDir, "C:\\Users\\xiaozhou\\Desktop\\Inject_V1.0_b1");
            strcpy(testParams->JuiceLoaderJarPath, "C:\\Users\\xiaozhou\\Desktop\\Inject_V1.0_b1\\JuiceLoader.jar");
            lpReserved = (void*)testParams;

            // Check if lpReserved is NULL
            if (lpReserved == nullptr) {
                PLOGE << "lpReserved is NULL";
                return FALSE;
            }

            // Allocate memory for local param
            InjectParams* pParams = (InjectParams*)malloc(sizeof(InjectParams));
            if (pParams == nullptr) {
                PLOGE << "Failed to allocate memory for local param";
                return FALSE;
            }
            
            // Copy lpReserved to local param
            memcpy(pParams, lpReserved, sizeof(InjectParams));

            // Create a thread to handle the injection
            HANDLE hThread = CreateThread(NULL, 0, ThreadProc, pParams, 0, NULL);
            if (hThread) {
                CloseHandle(hThread);
            } else {
                PLOGE << "Failed to create thread";
                free(pParams);
            }

            break;
        }

        case DLL_PROCESS_DETACH:
            PLOGI << "DLL_PROCESS_DETACH";
            break;
    }

    return TRUE;
}
