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
        log_error("BootstrapAPIPath is not a string.");
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
        log_error("JuiceLoaderJarPath is not a string.");
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
        log_error("JuiceLoaderLibPath is not a string.");
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
        log_error("EntryJarPath is not a string.");
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
        log_error("EntryClass is not a string.");
        return false;
    }
    if (EntryClass.u.s == NULL || strlen(EntryClass.u.s) == 0) {
        safe_copy(InjectionInfo.EntryClass, "cn.xiaozhou233.juiceloader.entry.Entry", INJECT_PATH_MAX);
    } else {
        safe_copy(InjectionInfo.EntryClass, EntryClass.u.s, INJECT_PATH_MAX);
    }

    toml_datum_t EntryMethod = toml_seek(InjectionTree, "EntryMethod");
    if (EntryMethod.type != TOML_STRING) {
        log_error("EntryMethod is not a string.");
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
