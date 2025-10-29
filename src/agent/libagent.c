/*
Author: xiaozhou233 (xiaozhou233@vip.qq.com)
File: libagent.c 
Desc: Entry point for the agent
*/

#include "ReflectiveLoader.h"
#include "windows.h"
#include "unistd.h"
#include "stdio.h"
#include "jni.h"
#include "jvmti.h"
#include "InjectParameters.h"
#include "log.h"
#include "GlobalUtils.h"
#include "shlwapi.h"
#include "tomlc17.h"
#include "datautils.h"
#include "juiceloader.h"

#define WIN_X64
extern HINSTANCE hAppInstance;

InjectionInfoType InjectionInfo;

static void initLogger() {
    log_set_level(LOG_TRACE);
    log_is_log_time(false);
    log_set_name("libagent");
    log_info("Logger initialized.");
}

DWORD WINAPI ThreadProc(LPVOID lpParam) {
    log_trace("New thread started.");

    memset(&InjectionInfo, 0, sizeof(InjectionInfo));

    /// ======== Check Environment ======== ///
    InjectParameters *param = (InjectParameters*)lpParam;
    if (!param) {
        log_error("ThreadProc got NULL param");
        return 1;
    }
    if (!param->ConfigDir || param->ConfigDir[0] == '\0') {
        log_error("ThreadProc got NULL or empty ConfigPath");
        return 1;
    }

    char ConfigPath[INJECT_PATH_MAX];
    snprintf(ConfigPath, sizeof(ConfigPath), "%s\\AgentConfig.toml", param->ConfigDir);
    toml_result_t toml_result = toml_parse_file_ex(ConfigPath);
    if (!toml_result.ok) {
        log_error("ThreadProc failed to parse config file: %s", toml_result.errmsg);
        return 1;
    }
    if (!ReadInjectionInfo(toml_result, param)) {
         log_error("Failed to read injection info"); 
         return 1; 
    }
    toml_free(toml_result);

    log_trace("Checking bootstrap-api.jar");
    if (GetFileAttributesA(InjectionInfo.BootstrapAPIPath) == INVALID_FILE_ATTRIBUTES) {
        log_error("bootstrap-api.jar not found: %s", InjectionInfo.BootstrapAPIPath);
        // MessageBoxA(NULL, "bootstrap-api.jar not found", "Error", MB_OK);
        return 1;
    }

    // Check JuiceLoader.jar
    
    log_trace("Checking JuiceLoader.jar");
    if (GetFileAttributesA(InjectionInfo.JuiceLoaderJarPath) == INVALID_FILE_ATTRIBUTES) {
        log_error("JuiceLoader.jar not found: %s", InjectionInfo.JuiceLoaderJarPath);
        // MessageBoxA(NULL, "JuiceLoader.jar not found", "Error", MB_OK);
        return 1;
    }
    /// ======== Check Environment ======== ///
    
    // Init vars
    jint result = JNI_ERR;
    JavaVM *jvm = NULL;
    JNIEnv *env = NULL;
    jvmtiEnv *jvmti = NULL;

    result = GetCreatedJVM(&jvm);
    if (result != JNI_OK) {
        log_error("Failed to get JVM (%d)", result);
        MessageBoxA(NULL, "Failed to get JVM", "Error", MB_OK);
        return 1;
    }
    log_info("Got JVM");


    result = GetJNIEnv(jvm, &env);
    if (result != JNI_OK) {
        log_error("Failed to get JNIEnv (%d)", result);
        // MessageBoxA(NULL, "Failed to get JNIEnv", "Error", MB_OK);
        return 1;
    } 
    log_info("Got JNIEnv");
    
    // Debug: JVM version
    jint version = (*env)->GetVersion(env);
    log_trace("JVM version: 0x%x", version);

    // Init and enable jvmti
    result = GetJVMTI(jvm, &jvmti);
    if (result != JNI_OK) {
        log_error("Failed to get JVMTI (%d)", result);
        // MessageBoxA(NULL, "Failed to get JVMTI", "Error", MB_OK);
        return 1;
    }
    log_info("Enabled jvmti");

    // Inject jars
    log_info("injecting bootstrap-api jar...");
    (*jvmti)->AddToSystemClassLoaderSearch(jvmti, InjectionInfo.BootstrapAPIPath);
    log_info("injected bootstrap-api jar");
    
    log_info("injecting loader jar...");
    (*jvmti)->AddToSystemClassLoaderSearch(jvmti, InjectionInfo.JuiceLoaderJarPath);
    log_info("injected loader jar");

    // init juiceloader
    log_info("init juice loader...");
    init_juiceloader(env, jvmti);
    
    initLogger();
    log_info("juice loader init done");

    // Invoke loader.init(JuiceLoaderLibPath, EntryJarPath)
    log_info("invoking loader init()...");
    log_info("============ Loader Info ============\n");
    jclass cls = (*env)->FindClass(env, "cn/xiaozhou233/juiceloader/JuiceLoader");
    if (cls == NULL) {
        log_error("FindClass JuiceLoader failed");
        return 1;
    }
    jmethodID mid = (*env)->GetStaticMethodID(env, cls, "init", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
    if (mid == NULL) {
        log_error("GetStaticMethodID init failed");
        if ((*env)->ExceptionCheck(env)) {
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
        }

        return 1;
    }
    (*env)->CallStaticVoidMethod(env, cls, mid, 
        (*env)->NewStringUTF(env, InjectionInfo.JuiceLoaderLibPath),
        (*env)->NewStringUTF(env, InjectionInfo.EntryJarPath),
        (*env)->NewStringUTF(env, InjectionInfo.EntryClass),
        (*env)->NewStringUTF(env, InjectionInfo.EntryMethod));
    log_info("============ Loader Info =============\n");
    log_info("invoked. ");
    log_info("done. cleaning up...");

    // :(
    // (*jvm)->DetachCurrentThread(jvm);

    free(param);
    log_info("exit.");
    return 0;
}

/// =========================================================== ///
/// Function: DllMain
/// Description: DLL Injection Entry Point
/// Inject Method: A) ReflectiveDLLInjection B) LoadLibraryA+CreateRemoteThread
/// =========================================================== ///
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpReserved){
    BOOL bReturnValue = TRUE;
    switch (dwReason){
        case DLL_QUERY_HMODULE:
            if (lpReserved != NULL)
                *(HMODULE *)lpReserved = hAppInstance;
            break;

        case DLL_PROCESS_ATTACH:
            hAppInstance = hinstDLL;

            // Initialize log
            initLogger();
            log_info("DLL_PROCESS_ATTACH");
            log_info("[*] JuiceAgent Version %d.%d Build %d", PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_BUILD_NUMBER);

            DisableThreadLibraryCalls(hinstDLL);
            
            // JVM Options Inject Method Detection
            // TODO: Remove Duplicated Code: GetCreatedJVM/GetEnv and var jvm/env (Duplicated at: ThreadProc)
            JavaVM *jvm = NULL;
            JNIEnv *env = NULL;
            jint result = GetCreatedJVM(&jvm);
            // Check if JVM is created
            if (result != JNI_OK || jvm == NULL) {
                log_error("Failed to get JVM (%d) [JVM: %p]", result, jvm);
                MessageBoxA(NULL, "Failed to get JVM [GetCreatedJVM Faliled or JVM is NULL]\nCheck console for more info.", "Error", MB_OK);
                return 1;
            }
            log_info("DllMain GetCreatedJVM OK. Checking if JVM is inited...");
            // Check if JVM is inited
            // JNI_EDETACHED: jvm not initialized, means Agent loaded before JVM inited (jvm options: -agentpath)
            // Will exit DllMain and jvm will invoke Agent_OnLoad
            if (GetJNIEnv(jvm, &env) == JNI_EDETACHED) {
                log_warn("Inject Method JVM Options Detected! Exiting...");
                log_trace("DllMain(DLL_PROCESS_ATTACH) Exiting...");
                log_info("Agent Will be injected by the Inject Methood JVM Options.\n");
                break;
            }

            // Allocate memory for local param
            InjectParameters *localParm = (InjectParameters*)malloc(sizeof(InjectParameters));
            if (localParm == NULL) {
                log_error("malloc failed for InjectParameters");
                break;
            }
            memset(localParm, 0, sizeof(InjectParameters));

            // Check if lpReserved is not NULL
            if (lpReserved != NULL) {
                // Copy remote param to local param
                InjectParameters *remoteParm = (InjectParameters*)lpReserved;

                safe_copy(localParm->ConfigDir, remoteParm->ConfigDir, sizeof(localParm->ConfigDir));
            } else {
                log_info("RemoteParm is NULL!");

                char DllDir[INJECT_PATH_MAX] = {0};

                if (GetModuleFileNameA(hinstDLL, DllDir, INJECT_PATH_MAX) == 0) {
                    // Failed to get module file name
                    DWORD err = GetLastError();
                    if (err == ERROR_MOD_NOT_FOUND) {
                        // Reflective detected, lpReserved must be set.
                        log_fatal("Reflective load detected! Please set ConfigDir manually! Exit.");
                    } else {
                        log_error("Normal Inject: GetModuleFileNameA failed! Error: %lu", err);
                    }
                    break;
                } else {
                    // Normal Inject(e.g. CreateRemoteThread+LoadLibraryA), lpReserved can be NULL.
                    PathRemoveFileSpecA(DllDir);
                    log_info("GetModuleFileNameA: %s", DllDir);
                    safe_copy(localParm->ConfigDir, DllDir, INJECT_PATH_MAX);
                }
            }

            log_info("ConfigDir: %s", localParm->ConfigDir);

            HANDLE hThread = CreateThread(NULL, 0, ThreadProc, localParm, 0, NULL);
            if (hThread) {
                CloseHandle(hThread);
            } else {
                log_error("CreateThread failed %lu", GetLastError());
                free(localParm);
            }

            break;
        case DLL_PROCESS_DETACH:
            log_info("DLL_PROCESS_DETACH");
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }
    return bReturnValue;
}

/// =========================================================== ///
/// Inject Method: JVM Options -agentpath

/// =========================================================== ///
/// Function: vm_init
/// Description: Called when the JVM is fully initialized. This is a callback function.
/// =========================================================== ///
static char *agent_options = NULL;
static void JNICALL vm_init(jvmtiEnv *jvmti, JNIEnv *env, jthread thread) {
    log_info("vminit: VM fully initialized!");
    log_info("Use options: %s", agent_options);

    log_info("JNIEnv: %p", env);
    log_info("jvmtiEnv: %p", jvmti);

    log_info("Version: %d", (*env)->GetVersion(env));

    // TODO: Remove Duplicated Code

    // Check  if agent_options is null
    if (agent_options == NULL) {
        // Use DLL path as default
        log_info("agent_options is null, use DLL path as default");
        char DllDir[INJECT_PATH_MAX] = {0};

        if (GetModuleFileNameA(hAppInstance, DllDir, INJECT_PATH_MAX) == 0) {
            // Failed to get module file name
            DWORD err = GetLastError();
            if (err == ERROR_MOD_NOT_FOUND) {
                // Reflective detected, lpReserved must be set.
                log_fatal("Reflective load detected! Please set ConfigDir manually! Exit.");
            } else {
                log_error("Normal Inject: GetModuleFileNameA failed! Error: %lu", err);
            }
                return;
        } else {
            // Normal Inject(e.g. CreateRemoteThread+LoadLibraryA), lpReserved can be NULL.
            PathRemoveFileSpecA(DllDir);
            log_info("GetModuleFileNameA: %s", DllDir);
            agent_options = DllDir;
        }
    }
    log_trace("agent_options: %s", agent_options);

    // Allocate memory for InjectParameters
    InjectParameters *localParm = (InjectParameters*)malloc(sizeof(InjectParameters));
    if (localParm == NULL) {
        log_error("malloc failed for InjectParameters");
        return;
    }
    memset(localParm, 0, sizeof(InjectParameters));

    // safe copy
    safe_copy(localParm->ConfigDir, agent_options, INJECT_PATH_MAX);

    // Start new thread
    HANDLE hThread = CreateThread(NULL, 0, ThreadProc, localParm, 0, NULL);
    if (hThread) {
        CloseHandle(hThread);
    } else {
        log_error("CreateThread failed %lu", GetLastError());
        free(localParm);
    }
}

/// =========================================================== ///
/// Function: Agent_OnLoad
/// Description: Java Agent OnLoad
/// Inject Method: JVM Options A) -agentpath:<path-to-agent>.dll=<param-config-dir>
/// Example: java -agentpath:./libagent.dll=./config -jar myapp.jar
/// =========================================================== ///

// Notice: This function is called by the JVM when the agent is loaded.
// Warning: JVM not initialized yet when this function is called.
JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved) {
    initLogger();
    log_info("Agent_OnLoad");
    log_info("Agent_OnLoad: options: %s", options);

    // save options
    if (options != NULL) {
        agent_options = strdup(options);
    }
    
    jvmtiEnv *jvmti;
    (*vm)->GetEnv(vm, (void **)&jvmti, JVMTI_VERSION_1_2);

    // callbacks
    jvmtiEventCallbacks callbacks = {0};
    callbacks.VMInit = &vm_init;
    (*jvmti)->SetEventCallbacks(jvmti, &callbacks, sizeof(callbacks));

    // enable events
    (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, NULL);

    log_info("Agent_OnLoad: Done, waiting for JVM Init event.");
    return JNI_OK;
}

/// Inject Method: JVM Options -agentpath
/// =========================================================== ///

// Notice: This function is useless but required by the JVM (in Windows).
JNIEXPORT jint JNICALL Agent_OnAttach (JavaVM* vm, char *options, void *reserved) {
    log_info("Agent_OnAttach");
    #ifdef WIN_X64
        log_warn("Agent_OnAttach: Windows will invoke DllMain, Exiting Agent_OnAttach.");
        log_info("Agent_OnAttach will exit in Windows, Loading will be done in DllMain.\n");
    #endif
    return JNI_OK;
}