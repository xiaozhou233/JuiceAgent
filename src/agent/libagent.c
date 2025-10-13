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

#define LOG_PREFIX "[JuiceAgent]"

DWORD WINAPI ThreadProc(LPVOID lpParam) {
    log_trace("%s New thread started.", LOG_PREFIX);

    /// ======== Check Environment ======== ///
    InjectParameters *param = (InjectParameters*)lpParam;
    if (!param) {
        log_error("%s ThreadProc got NULL param", LOG_PREFIX);
        return 1;
    }

    log_info("%s ThreadProc got param: \nJuiceLoaderJarPath: %s\nJuiceLoaderLibPath: %s\nEntryJarPath: %s\nBootstrapApiPath: %s", LOG_PREFIX,
              param->JuiceLoaderJarPath,
              param->JuiceLoaderLibPath,
              param->EntryJarPath,
              param->BootstrapApiPath);

    const char* bootstrapApiJar = param->BootstrapApiPath;
    if (bootstrapApiJar == NULL) {
        log_error("%s BootstrapApiPath is not set", LOG_PREFIX);
        MessageBoxA(NULL, "BootstrapApiPath is not set", "Error", MB_OK);
        return 1;
    }

    log_trace("%s Checking bootstrap-api.jar", LOG_PREFIX);
    if (access(bootstrapApiJar, 0) == -1) {
        log_error("%s bootstrap-api.jar not found", LOG_PREFIX);
        MessageBoxA(NULL, "bootstrap-api.jar not found", "Error", MB_OK);
        return 1;
    }

    const char* loaderJar = param->JuiceLoaderJarPath;
    if (loaderJar == NULL) {
        log_error("%s JuiceLoaderJarPath is not set", LOG_PREFIX);
        MessageBoxA(NULL, "JuiceLoaderJarPath is not set", "Error", MB_OK);
        return 1;
    }

    // Check JuiceLoader.jar
    log_trace("%s Checking JuiceLoader.jar", LOG_PREFIX);
    if (access(loaderJar, 0) == -1) {
        log_error("%s JuiceLoader.jar not found", LOG_PREFIX);
        MessageBoxA(NULL, "JuiceLoader.jar not found", "Error", MB_OK);
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
        MessageBoxA(NULL, "Failed to get JNIEnv", "Error", MB_OK);
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
        MessageBoxA(NULL, "Failed to get JVMTI", "Error", MB_OK);
        return 1;
    }
    log_info("%s Enabled jvmti", LOG_PREFIX);

    // Inject jars
    log_info("%s injecting bootstrap-api jar...", LOG_PREFIX);
    (*jvmti)->AddToBootstrapClassLoaderSearch(jvmti, bootstrapApiJar);
    log_info("%s injected bootstrap-api jar", LOG_PREFIX);
    
    log_info("%s injecting loader jar...", LOG_PREFIX);
    (*jvmti)->AddToBootstrapClassLoaderSearch(jvmti, loaderJar);
    log_info("%s injected loader jar", LOG_PREFIX);

    // Invoke loader.init(JuiceLoaderLibPath, EntryJarPath)
    log_info("%s invoking loader init()...", LOG_PREFIX);
    log_info("\n============ Loader Info ============\n");
    jclass cls = (*env)->FindClass(env, "cn/xiaozhou233/juiceloader/JuiceLoader");
    jmethodID mid = (*env)->GetStaticMethodID(env, cls, "init", "(Ljava/lang/String;Ljava/lang/String;)V");
    (*env)->CallStaticVoidMethod(env, cls, mid, 
        (*env)->NewStringUTF(env, param->JuiceLoaderLibPath),
        (*env)->NewStringUTF(env, param->EntryJarPath));
    log_info("\n============ Loader Info =============\n");
    log_info("%s invoked. ", LOG_PREFIX);
    log_info("%s done. cleaning up...", LOG_PREFIX);

    free(param);
    (*jvm)->DetachCurrentThread(jvm);

    log_info("%s exit.", LOG_PREFIX);
    return 0;
}

void safe_copy(char *dest, const char *src, size_t dest_size) {
    if (!dest || dest_size == 0) return;
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
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
            log_info("%s DLL_PROCESS_ATTACH", LOG_PREFIX);

            DisableThreadLibraryCalls(hinstDLL);

            if (lpReserved != NULL) {
                InjectParameters *remoteParm = (InjectParameters*)lpReserved;

                InjectParameters *localParm = (InjectParameters*)malloc(sizeof(InjectParameters));
                if (localParm == NULL) {
                    log_error("%s malloc failed for InjectParameters", LOG_PREFIX);
                    break;
                }

                memset(localParm, 0, sizeof(InjectParameters));

                safe_copy(localParm->JuiceLoaderJarPath, remoteParm->JuiceLoaderJarPath, sizeof(localParm->JuiceLoaderJarPath));
                safe_copy(localParm->JuiceLoaderLibPath, remoteParm->JuiceLoaderLibPath, sizeof(localParm->JuiceLoaderLibPath));
                safe_copy(localParm->EntryJarPath, remoteParm->EntryJarPath, sizeof(localParm->EntryJarPath));
                safe_copy(localParm->BootstrapApiPath, remoteParm->BootstrapApiPath, sizeof(localParm->BootstrapApiPath));

                log_info("%s DllMain got param: \nJuiceLoaderJarPath: %s\nJuiceLoaderLibPath: %s\nEntryJarPath: %s\nBootstrapApiPath: %s", LOG_PREFIX,
                    localParm->JuiceLoaderJarPath,
                    localParm->JuiceLoaderLibPath,
                    localParm->EntryJarPath,
                    localParm->BootstrapApiPath);

                HANDLE hThread = CreateThread(NULL, 0, ThreadProc, localParm, 0, NULL);
                if (hThread) {
                    CloseHandle(hThread);
                } else {
                    log_error("%s CreateThread failed %lu", LOG_PREFIX, GetLastError());
                    free(localParm);
                }
            } else {
                log_error("%s RemoteParm is NULL!", LOG_PREFIX);
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
