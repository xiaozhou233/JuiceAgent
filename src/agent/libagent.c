#include "ReflectiveLoader.h"
#include "windows.h"
#include "unistd.h"
#include "stdio.h"
#include "jni.h"
#include "jvmti.h"
#include "InjectParameters.h"
#include "log.h"
#include "GlobalUtils.h"

#define WIN_X64
extern HINSTANCE hAppInstance;

#define LOGNAME "libagent"

DWORD WINAPI ThreadProc(LPVOID lpParam) {
    log_trace("[%s] New thread started.", LOGNAME);

    /// ======== Check Environment ======== ///
    InjectParameters *param = (InjectParameters*)lpParam;
    if (!param) {
        log_error("[%s] ThreadProc got NULL param", LOGNAME);
        return 1;
    }

    log_info("[%s] ThreadProc got param: %s", LOGNAME, param->loaderDir);

    const char* dir = param->loaderDir;
    log_info("[%s] LoaderDir: %s", LOGNAME, dir);
    if (dir == NULL) {
        log_error("[%s] LoaderDir is not set", LOGNAME);
        MessageBoxA(NULL, "LoaderDir is not set", "Error", MB_OK);
        return 1;
    }

    // Check JuiceLoader.jar
    log_trace("[%s] Checking JuiceLoader.jar", LOGNAME);
    char path[MAX_PATH];
    sprintf(path, "%s\\JuiceLoader.jar", dir);
    if (access(path, 0) == -1) {
        log_error("[%s] JuiceLoader.jar not found", LOGNAME);
        MessageBoxA(NULL, "JuiceLoader.jar not found", "Error", MB_OK);
        return 1;
    }
    /// ======== Check Environment ======== ///
    
    // Init vars
    jint result = JNI_ERR;
    JavaVM *jvm = NULL;
    
    result = GetCreatedJVM(&jvm);
    if (result != JNI_OK) {
        log_error("[%s] Failed to get JVM (%d)", LOGNAME, result);
        MessageBoxA(NULL, "Failed to get JVM", "Error", MB_OK);
        return 1;
    }
    log_info("[%s] Got JVM", LOGNAME);

    JNIEnv *env = NULL;
    result = GetJNIEnv(jvm, &env);
    if (result != JNI_OK) {
        log_error("[%s] Failed to get JNIEnv (%d)", LOGNAME, result);
        MessageBoxA(NULL, "Failed to get JNIEnv", "Error", MB_OK);
        return 1;
    } 
    log_info("[%s] Got JNIEnv", LOGNAME);
    

    // Debug: JVM version
    jint version = (*env)->GetVersion(env);
    log_trace("[%s] JVM version: 0x%x", LOGNAME, version);

    // Init and enable jvmti
    jvmtiEnv *jvmti = NULL;
    result = GetJVMTI(jvm, &jvmti);
    if (result != JNI_OK) {
        log_error("[%s] Failed to get JVMTI (%d)", LOGNAME, result);
        MessageBoxA(NULL, "Failed to get JVMTI", "Error", MB_OK);
        return 1;
    }
    log_info("[%s] enabled jvmti", LOGNAME);

    // Inject jars
    log_info("[%s] injecting loader jar...", LOGNAME);
    (*jvmti)->AddToBootstrapClassLoaderSearch(jvmti, path);
    log_info("[%s] injected loader jar", LOGNAME);

    // Invoke init(dir)
    log_info("[%s] invoking loader init()...", LOGNAME);
    log_info("\n============ Loader Info ============\n");
    jclass cls = (*env)->FindClass(env, "cn/xiaozhou233/juiceloader/JuiceLoader");
    jmethodID mid = (*env)->GetStaticMethodID(env, cls, "init", "(Ljava/lang/String;)V");
    (*env)->CallStaticVoidMethod(env, cls, mid, (*env)->NewStringUTF(env, dir));
    log_info("\n============ Loader Info =============\n");
    log_info("[%s] invoked. ", LOGNAME);
    log_info("[%s] done. cleaning up...", LOGNAME);

    free(param);
    (*jvm)->DetachCurrentThread(jvm);

    log_info("[%s] exit.", LOGNAME);
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

            log_set_level(LOG_TRACE);
            log_is_log_filename(false);
            log_is_log_line(false);
            log_is_log_time(false);
            log_info("[%s] DLL_PROCESS_ATTACH", LOGNAME);

            DisableThreadLibraryCalls(hinstDLL);

            if (lpReserved != NULL) {
                InjectParameters *remoteParm = (InjectParameters*)lpReserved;

                InjectParameters *localParm = (InjectParameters*)malloc(sizeof(InjectParameters));
                if (localParm == NULL) {
                    log_error("[%s] malloc failed for InjectParameters", LOGNAME);
                    break;
                }

                memset(localParm, 0, sizeof(InjectParameters));

                strncpy(localParm->loaderDir, remoteParm->loaderDir, sizeof(localParm->loaderDir) - 1);
                localParm->loaderDir[sizeof(localParm->loaderDir)-1] = '\0';

                log_info("[%s] DllMain got Loader Dir = %s", LOGNAME, localParm->loaderDir);

                HANDLE hThread = CreateThread(NULL, 0, ThreadProc, localParm, 0, NULL);
                if (hThread) {
                    CloseHandle(hThread);
                } else {
                    log_error("[%s] CreateThread failed %lu", LOGNAME, GetLastError());
                    free(localParm);
                }
            } else {
                log_error("[%s] RemoteParm is NULL!", LOGNAME);
            }
            break;
            
        case DLL_PROCESS_DETACH:
            log_info("[%s] DLL_PROCESS_DETACH", LOGNAME);
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }
    return bReturnValue;
}
