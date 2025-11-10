#include <windows.h>
#include "Logger.hpp"
#include <JuiceAgent.h>
#include <tinytoml/toml.h>

// For Java VM
#include <jni.h>
#include <jvmti.h>

struct _JuiceAgent {
    bool isAttached;
    JavaVM *jvm;
    jvmtiEnv *jvmti;
    JNIEnv *env;
} JuiceAgent = {false, NULL, NULL, NULL};

InjectionInfoType InjectionInfo;


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

static int InvokeJuiceLoaderInit(const char* ConfigDir) {
    // Variables
    int result = JNI_ERR;
    JNIEnv *env = JuiceAgent.env;
    jvmtiEnv *jvmti = JuiceAgent.jvmti;

    const char *bootstrap_class = "cn/xiaozhou233/juiceloader/JuiceLoaderBootstrap";
    const char *method_name = "init";
    const char *method_signature = "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V";

    PLOGI << "Invoke JuiceLoader Init";
    PLOGI << "================ JuiceLoader Init =================";
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
    env->CallStaticVoidMethod(cls, mid, env->NewStringUTF(InjectionInfo.EntryJarPath),
                                        env->NewStringUTF(InjectionInfo.EntryClass),
                                        env->NewStringUTF(InjectionInfo.EntryMethod),
                                        env->NewStringUTF(InjectionInfo.InjectionDir),
                                        env->NewStringUTF(InjectionInfo.JuiceLoaderLibraryPath));
    if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
    }

    PLOGI << "================ JuiceLoader Init =================";

    PLOGI << "Invoke JuiceLoader Init done";
    return 0;
}

static int ReadConfig(const char* ConfigDir)
{
    // ConfigDir/config.toml
    std::string path = std::string(ConfigDir) + "\\config.toml";
    std::ifstream ifs(path);
    if (!ifs) return -1;

    // Parse toml
    toml::ParseResult pr = toml::parse(ifs);
    if (!pr.valid()) return -2;

    const toml::Value& tbl = pr.value;

    // Helper function: get string from toml
    auto get_str = [&](const char* key, const char* def, char* out) {
        const toml::Value* v = tbl.find(key);
        if (v && v->is<std::string>()) {
            std::string s = v->as<std::string>();
            if (!s.empty()) {
                strncpy(out, s.c_str(), INJECT_PATH_MAX - 1);
                PLOGD << "Found parameter: " << key << " = " << s.c_str();
            } else {
            strncpy(out, def, INJECT_PATH_MAX - 1);
            PLOGD << "Parameter " << key << " is empty, use default: " << def;
            }
        } else {
            strncpy(out, def, INJECT_PATH_MAX - 1);
            PLOGD << "Not found parameter: " << key << ", use default: " << def;
        }
        out[INJECT_PATH_MAX - 1] = '\0';
    };

    // Default values
    std::string defaultJuiceLoader = std::string(ConfigDir) + "\\JuiceLoader.jar";
    std::string defaultJuiceLoaderLibrary = std::string(ConfigDir) + "\\libjuiceloader.dll";
    std::string defaultEntryJar    = std::string(ConfigDir) + "\\Entry.jar";
    std::string defaultInjectionDir = std::string(ConfigDir) + "\\injection";

    get_str("JuiceLoaderJarPath", defaultJuiceLoader.c_str(), InjectionInfo.JuiceLoaderJarPath);
    get_str("JuiceLoaderLibraryPath", defaultJuiceLoaderLibrary.c_str(), InjectionInfo.JuiceLoaderLibraryPath);
    get_str("InjectionDir", defaultInjectionDir.c_str(), InjectionInfo.InjectionDir);
    get_str("EntryJarPath", defaultEntryJar.c_str(), InjectionInfo.EntryJarPath);
    get_str("EntryClass", "cn.xiaozhou233.test.Entry", InjectionInfo.EntryClass);
    get_str("EntryMethod", "start", InjectionInfo.EntryMethod);

    PLOGD << "JuiceLoaderJarPath: " << InjectionInfo.JuiceLoaderJarPath;
    PLOGD << "JuiceLoaderLibraryPath: " << InjectionInfo.JuiceLoaderLibraryPath;
    PLOGD << "InjectionDir: " << InjectionInfo.InjectionDir;
    PLOGD << "EntryJarPath: " << InjectionInfo.EntryJarPath;
    PLOGD << "EntryClass: " << InjectionInfo.EntryClass;
    PLOGD << "EntryMethod: " << InjectionInfo.EntryMethod;

    return 0;
}


int InitJuiceAgent(const char* ConfigDir) {
    // Variables
    int result = JNI_ERR;

    // Step 0: Read Config
    if (ReadConfig(ConfigDir) != 0) {
        PLOGE << "Failed to read config";
        return 1;
    }
    
    // Step 1: Get the Java environment
    if (GetJavaEnv() != 0) {
        PLOGE << "Failed to get Java environment code=" << result;
        return 1;
    }

    // Step 2: Inject JuiceLoader.jar
    result = JuiceAgent.jvmti->AddToSystemClassLoaderSearch(InjectionInfo.JuiceLoaderJarPath);
    if (result != JNI_OK) {
        PLOGE.printf("AddToSystemClassLoaderSearch failed: %d", result);
        return 1;
    }

    // Step 3: Invoke JuiceLoader Init
    if (InvokeJuiceLoaderInit(ConfigDir) != 0) {
        PLOGE << "Failed to invoke JuiceLoader Init code=" << result;
        return 1;
    }

    // Step 4: Done, cleanup

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

    PLOGI << "Init JuiceAgent...";
    int result = InitJuiceAgent(pParams->ConfigDir);
    PLOGI.printf("Init JuiceAgent result: %d", result);

    PLOGI << "Exit ThreadProc";
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH: {
            InitLogger();

            PLOGI << "DLL_PROCESS_ATTACH";
            PLOGI.printf("[*] JuiceAgent Version %d.%d Build %d", PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_BUILD_NUMBER);

            DisableThreadLibraryCalls(hinstDLL);

            // TODO: ReflectiveLoader Params
            InjectParams *testParams = (InjectParams*)malloc(sizeof(InjectParams));
            strcpy(testParams->ConfigDir, "C:\\Users\\xiaozhou\\Desktop\\Inject_V1.0_b1");
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

extern "C" __declspec(dllexport)
JNIEXPORT jint JNICALL 
Agent_OnLoad(JavaVM* vm, char *options, void *reserved) {
    return JNI_OK;
}

extern "C" __declspec(dllexport)
JNIEXPORT jint JNICALL 
Agent_OnAttach(JavaVM* vm, char *options, void *reserved) {
    return JNI_OK;
}