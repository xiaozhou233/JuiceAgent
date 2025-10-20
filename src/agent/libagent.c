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

#define WIN_X64
extern HINSTANCE hAppInstance;

#define LOG_PREFIX "[JuiceAgent]"

struct _InjectionInfo {
    char BootstrapAPIPath[INJECT_PATH_MAX];
    char JuiceLoaderJarPath[INJECT_PATH_MAX];
    char JuiceLoaderLibPath[INJECT_PATH_MAX];
    char EntryJarPath[INJECT_PATH_MAX];
    char EntryClass[INJECT_PATH_MAX];
    char EntryMethod[INJECT_PATH_MAX];
} InjectionInfo;

void safe_copy(char *dest, const char *src, size_t dest_size) {
    if (!dest || dest_size == 0) return;
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
}

static bool ReadInjectionInfo(toml_result_t result, InjectParameters* params) {
    toml_datum_t InjectionTree = toml_seek(result.toptab, "Injection");

    // ======= BootstrapAPIPath =======
    toml_datum_t BootstrapAPIPath = toml_seek(InjectionTree, "BootstrapAPIPath");
    char BootstrapAPIPathStr[INJECT_PATH_MAX];
    if (BootstrapAPIPath.type != TOML_STRING) {
        log_error("%s BootstrapAPIPath is not a string.", LOG_PREFIX);
        return false;
    }
    if (BootstrapAPIPath.u.s == NULL || strlen(BootstrapAPIPath.u.s) == 0) {
        sprintf(BootstrapAPIPathStr, "%s\\bootstrap-api.jar", params->ConfigDir);
    } else {
        sprintf(BootstrapAPIPathStr, "%s", BootstrapAPIPath.u.s);
    }
    safe_copy(InjectionInfo.BootstrapAPIPath, BootstrapAPIPathStr, INJECT_PATH_MAX);
    // ======= BootstrapAPIPath =======

    // ======= JuiceLoaderJarPath =======
    toml_datum_t JuiceLoaderJarPath = toml_seek(InjectionTree, "JuiceLoaderJarPath");
    char JuiceLoaderJarPathStr[INJECT_PATH_MAX];
    if (JuiceLoaderJarPath.type != TOML_STRING) {
        log_error("%s JuiceLoaderJarPath is not a string.", LOG_PREFIX);
        return false;
    }
    if (JuiceLoaderJarPath.u.s == NULL || strlen(JuiceLoaderJarPath.u.s) == 0) {
        sprintf(JuiceLoaderJarPathStr, "%s\\JuiceLoader.jar", params->ConfigDir);
    } else {
        sprintf(JuiceLoaderJarPathStr, "%s", JuiceLoaderJarPath.u.s);
    }
    safe_copy(InjectionInfo.JuiceLoaderJarPath, JuiceLoaderJarPathStr, INJECT_PATH_MAX);
    // ======= JuiceLoaderJarPath =======

    // ======= JuiceLoaderLibPath =======
    toml_datum_t JuiceLoaderLibPath = toml_seek(InjectionTree, "JuiceLoaderLibPath");
    char JuiceLoaderLibPathStr[INJECT_PATH_MAX];
    if (JuiceLoaderLibPath.type != TOML_STRING) {
        log_error("%s JuiceLoaderLibPath is not a string.", LOG_PREFIX);
        return false;
    }
    if (JuiceLoaderLibPath.u.s == NULL || strlen(JuiceLoaderLibPath.u.s) == 0) {
        sprintf(JuiceLoaderLibPathStr, "%s\\libjuiceloader.dll", params->ConfigDir);
    } else {
        sprintf(JuiceLoaderLibPathStr, "%s", JuiceLoaderLibPath.u.s);
    }
    safe_copy(InjectionInfo.JuiceLoaderLibPath, JuiceLoaderLibPathStr, INJECT_PATH_MAX);
    // ======= JuiceLoaderLibPath =======

    // ======= Entry =======
    toml_datum_t EntryJarPath = toml_seek(InjectionTree, "EntryJarPath");
    char EntryJarPathStr[INJECT_PATH_MAX];
    if (EntryJarPath.type != TOML_STRING) {
        log_error("%s EntryJarPath is not a string.", LOG_PREFIX);
        return false;
    }
    if (EntryJarPath.u.s == NULL || strlen(EntryJarPath.u.s) == 0) {
        sprintf(EntryJarPathStr, "%s\\entry.jar", params->ConfigDir);
    } else {
        sprintf(EntryJarPathStr, "%s", EntryJarPath.u.s);
    }
    safe_copy(InjectionInfo.EntryJarPath, EntryJarPathStr, INJECT_PATH_MAX);

    toml_datum_t EntryClass = toml_seek(InjectionTree, "EntryClass");
    if (EntryClass.type != TOML_STRING) {
        log_error("%s EntryClass is not a string.", LOG_PREFIX);
        return false;
    }
    if (EntryClass.u.s == NULL || strlen(EntryClass.u.s) == 0) {
        safe_copy(InjectionInfo.EntryClass, "cn.xiaozhou233.juiceloader.entry.Entry", INJECT_PATH_MAX);
    } else {
        safe_copy(InjectionInfo.EntryClass, EntryClass.u.s, INJECT_PATH_MAX);
    }

    toml_datum_t EntryMethod = toml_seek(InjectionTree, "EntryMethod");
    if (EntryMethod.type != TOML_STRING) {
        log_error("%s EntryMethod is not a string.", LOG_PREFIX);
        return false;
    }
    if (EntryMethod.u.s == NULL || strlen(EntryMethod.u.s) == 0) {
        safe_copy(InjectionInfo.EntryMethod, "start", INJECT_PATH_MAX);
    } else {
        safe_copy(InjectionInfo.EntryMethod, EntryMethod.u.s, INJECT_PATH_MAX);
    }
    // ======= Entry =======
    return true;
}

DWORD WINAPI ThreadProc(LPVOID lpParam) {
    log_trace("%s New thread started.", LOG_PREFIX);

    memset(&InjectionInfo, 0, sizeof(InjectionInfo));

    /// ======== Check Environment ======== ///
    InjectParameters *param = (InjectParameters*)lpParam;
    if (!param) {
        log_error("%s ThreadProc got NULL param", LOG_PREFIX);
        return 1;
    }
    if (!param->ConfigDir || param->ConfigDir[0] == '\0') {
        log_error("%s ThreadProc got NULL or empty ConfigPath", LOG_PREFIX);
        return 1;
    }

    char ConfigPath[INJECT_PATH_MAX];
    snprintf(ConfigPath, sizeof(ConfigPath), "%s\\AgentConfig.toml", param->ConfigDir);
    toml_result_t toml_result = toml_parse_file_ex(ConfigPath);
    if (!toml_result.ok) {
        log_error("%s ThreadProc failed to parse config file: %s", LOG_PREFIX, toml_result.errmsg);
        return 1;
    }
    if (!ReadInjectionInfo(toml_result, param)) {
         log_error("%s Failed to read injection info", LOG_PREFIX); 
         return 1; 
    }
    toml_free(toml_result);

    log_trace("%s Checking bootstrap-api.jar", LOG_PREFIX);
    if (GetFileAttributesA(InjectionInfo.BootstrapAPIPath) == INVALID_FILE_ATTRIBUTES) {
        log_error("%s bootstrap-api.jar not found: %s", LOG_PREFIX, InjectionInfo.BootstrapAPIPath);
        // MessageBoxA(NULL, "bootstrap-api.jar not found", "Error", MB_OK);
        return 1;
    }

    // Check JuiceLoader.jar
    
    log_trace("%s Checking JuiceLoader.jar", LOG_PREFIX);
    if (GetFileAttributesA(InjectionInfo.JuiceLoaderJarPath) == INVALID_FILE_ATTRIBUTES) {
        log_error("%s JuiceLoader.jar not found: %s", LOG_PREFIX, InjectionInfo.JuiceLoaderJarPath);
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
        log_error("%s Failed to get JVM (%d)", LOG_PREFIX, result);
        MessageBoxA(NULL, "Failed to get JVM", "Error", MB_OK);
        return 1;
    }
    log_info("%s Got JVM", LOG_PREFIX);


    result = GetJNIEnv(jvm, &env);
    if (result != JNI_OK) {
        log_error("%s Failed to get JNIEnv (%d)", LOG_PREFIX, result);
        // MessageBoxA(NULL, "Failed to get JNIEnv", "Error", MB_OK);
        return 1;
    } 
    log_info("%s Got JNIEnv", LOG_PREFIX);
    
    // Debug: JVM version
    jint version = (*env)->GetVersion(env);
    log_trace("%s JVM version: 0x%x", LOG_PREFIX, version);

    // Init and enable jvmti
    result = GetJVMTI(jvm, &jvmti);
    if (result != JNI_OK) {
        log_error("%s Failed to get JVMTI (%d)", LOG_PREFIX, result);
        // MessageBoxA(NULL, "Failed to get JVMTI", "Error", MB_OK);
        return 1;
    }
    log_info("%s Enabled jvmti", LOG_PREFIX);

    // Inject jars
    log_info("%s injecting bootstrap-api jar...", LOG_PREFIX);
    (*jvmti)->AddToSystemClassLoaderSearch(jvmti, InjectionInfo.BootstrapAPIPath);
    log_info("%s injected bootstrap-api jar", LOG_PREFIX);
    
    log_info("%s injecting loader jar...", LOG_PREFIX);
    (*jvmti)->AddToSystemClassLoaderSearch(jvmti, InjectionInfo.JuiceLoaderJarPath);
    log_info("%s injected loader jar", LOG_PREFIX);

    // Invoke loader.init(JuiceLoaderLibPath, EntryJarPath)
    log_info("%s invoking loader init()...", LOG_PREFIX);
    log_info("\n============ Loader Info ============\n");
    jclass cls = (*env)->FindClass(env, "cn/xiaozhou233/juiceloader/JuiceLoader");
    if (cls == NULL) {
        log_error("%s FindClass JuiceLoader failed", LOG_PREFIX);
        return 1;
    }
    jmethodID mid = (*env)->GetStaticMethodID(env, cls, "init", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
    if (mid == NULL) {
        log_error("%s GetStaticMethodID init failed", LOG_PREFIX);
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
    log_info("\n============ Loader Info =============\n");
    log_info("%s invoked. ", LOG_PREFIX);
    log_info("%s done. cleaning up...", LOG_PREFIX);

    (*jvm)->DetachCurrentThread(jvm);

    free(param);
    log_info("%s exit.", LOG_PREFIX);
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
            log_is_log_filename(false);
            log_is_log_line(false);
            log_is_log_time(false);
            log_info("%s DLL_PROCESS_ATTACH", LOG_PREFIX);
            log_info("[*] JuiceAgent Version %d.%d Build %d", PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_BUILD_NUMBER);

            DisableThreadLibraryCalls(hinstDLL);
            
            // Allocate memory for local param
            InjectParameters *localParm = (InjectParameters*)malloc(sizeof(InjectParameters));
            if (localParm == NULL) {
                log_error("%s malloc failed for InjectParameters", LOG_PREFIX);
                break;
            }
            memset(localParm, 0, sizeof(InjectParameters));

            // Check if lpReserved is not NULL
            if (lpReserved != NULL) {
                // Copy remote param to local param
                InjectParameters *remoteParm = (InjectParameters*)lpReserved;

                safe_copy(localParm->ConfigDir, remoteParm->ConfigDir, sizeof(localParm->ConfigDir));
            } else {
                log_info("%s RemoteParm is NULL!", LOG_PREFIX);

                char DllDir[INJECT_PATH_MAX] = {0};

                if (GetModuleFileNameA(hinstDLL, DllDir, INJECT_PATH_MAX) == 0) {
                    // Failed to get module file name
                    DWORD err = GetLastError();
                    if (err == ERROR_MOD_NOT_FOUND) {
                        // Reflective detected, lpReserved must be set.
                        log_fatal("%s Reflective load detected! Please set ConfigDir manually! Exit.", LOG_PREFIX);
                    } else {
                        log_error("%s Normal Inject: GetModuleFileNameA failed! Error: %lu", LOG_PREFIX, err);
                    }
                    break;
                } else {
                    // Normal Inject(e.g. CreateRemoteThread+LoadLibraryA), lpReserved can be NULL.
                    PathRemoveFileSpecA(DllDir);
                    log_info("%s GetModuleFileNameA: %s", LOG_PREFIX, DllDir);
                    safe_copy(localParm->ConfigDir, DllDir, INJECT_PATH_MAX);
                }
            }

            log_info("%s ConfigDir: %s", LOG_PREFIX, localParm->ConfigDir);

            HANDLE hThread = CreateThread(NULL, 0, ThreadProc, localParm, 0, NULL);
            if (hThread) {
                CloseHandle(hThread);
            } else {
                log_error("%s CreateThread failed %lu", LOG_PREFIX, GetLastError());
                free(localParm);
            }

            break;
        case DLL_PROCESS_DETACH:
            log_info("%s DLL_PROCESS_DETACH", LOG_PREFIX);
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }
    return bReturnValue;
}
