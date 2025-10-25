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

#define WIN_X64
extern HINSTANCE hAppInstance;

InjectionInfoType InjectionInfo;

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

    (*jvm)->DetachCurrentThread(jvm);

    free(param);
    log_info("exit.");
    return 0;
}

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
            log_set_level(LOG_TRACE);
            log_is_log_time(false);
            log_set_name("libagent");
            log_info("DLL_PROCESS_ATTACH");
            log_info("[*] JuiceAgent Version %d.%d Build %d", PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_BUILD_NUMBER);

            DisableThreadLibraryCalls(hinstDLL);
            
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
