#include "ReflectiveLoader.h"
#include "windows.h"
#include "unistd.h"
#include "stdio.h"
#include "jni.h"
#include "jvmti.h"
#include "InjectParameters.h"

#define WIN_X64
extern HINSTANCE hAppInstance;

DWORD WINAPI ThreadProc(LPVOID lpParam) {
    printf("libagent: New thread created\n");

    InjectParameters *param = (InjectParameters*)lpParam;
    if (!param) {
        printf("libagent: ThreadProc got NULL param\n");
        return 1;
    }

    printf("libagent: ThreadProc got param: %s\n", param->loaderDir);

    const char* dir = param->loaderDir;
    printf("libagent: LoaderDir: %s\n", dir);
    if (dir == NULL) {
        printf("libagent: LoaderDir is not set\n");
        MessageBoxA(NULL, "LoaderDir is not set", "Error", MB_OK);
        return 1;
    }

    // Check JuiceLoader.jar
    printf("libagent: Check JuiceLoader.jar");
    char path[MAX_PATH];
    sprintf(path, "%s\\JuiceLoader.jar", dir);
    if (access(path, 0) == -1) {
        printf("libagent: JuiceLoader.jar not found\n");
        MessageBoxA(NULL, "JuiceLoader.jar not found", "Error", MB_OK);
        return 1;
    }
    
    // Init vars
    JavaVM *jvm;
    JNIEnv *env;

    // Get JavaVM
    if (JNI_GetCreatedJavaVMs(&jvm, 1, NULL) != JNI_OK || !jvm) {
        printf("DllMain: JNI_GetCreatedJavaVMs failed\n");
        MessageBoxA(NULL, "JNI_GetCreatedJavaVMs failed", "Error", MB_OK);
    } else {
        printf("DllMain: JNI_GetCreatedJavaVMs success\n");
    }

    // Get JNIEnv
    if ((*jvm)->AttachCurrentThread(jvm, (void **)&env, NULL) != JNI_OK) {
        MessageBoxA(NULL, "Failed to attach current thread to JVM!", "Error", MB_ICONERROR);
        return 1;
    } else {
        printf("DllMain: AttachCurrentThread success\n");
    }

    // Debug: JVM version
    jint version = (*env)->GetVersion(env);
    printf("JVM version: 0x%x\n", version);

    // Init and enable jvmti
    printf("libagent: enabling jvmti\n");
    jvmtiEnv *jvmti;
    jint res = (*jvm)->GetEnv(jvm, (void**)&jvmti, JVMTI_VERSION_1_2);
    if (res != JNI_OK || jvmti == NULL) {
        printf("libagent: GetEnv failed\n");
        return 1;
    } else {
        printf("libagent: GetEnv success\n");
    }

    // Inject jars
    printf("libagent: injecting jars...\n");
    (*jvmti)->AddToBootstrapClassLoaderSearch(jvmti, path);

    // Invoke init(dir)
    printf("libagent: invoking init()...\n");
    jclass cls = (*env)->FindClass(env, "cn/xiaozhou233/juiceloader/JuiceLoader");
    jmethodID mid = (*env)->GetStaticMethodID(env, cls, "init", "(Ljava/lang/String;)V");
    (*env)->CallStaticVoidMethod(env, cls, mid, (*env)->NewStringUTF(env, dir));
    printf("done\n");

    free(param);

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
            printf("libagent: DLL_PROCESS_ATTACH\n");

            DisableThreadLibraryCalls(hinstDLL);

            if (lpReserved != NULL) {
                InjectParameters *remoteParm = (InjectParameters*)lpReserved;

                InjectParameters *localParm = (InjectParameters*)malloc(sizeof(InjectParameters));
                if (localParm == NULL) {
                    printf("libagent: malloc failed for InjectParameters\n");
                    break;
                }

                memset(localParm, 0, sizeof(InjectParameters));

                strncpy(localParm->loaderDir, remoteParm->loaderDir, sizeof(localParm->loaderDir) - 1);
                localParm->loaderDir[sizeof(localParm->loaderDir)-1] = '\0';

                printf("libagent: Loader Dir = %s\n", localParm->loaderDir);

                HANDLE hThread = CreateThread(NULL, 0, ThreadProc, localParm, 0, NULL);
                if (hThread) {
                    CloseHandle(hThread);
                } else {
                    printf("libagent: CreateThread failed %lu\n", GetLastError());
                    free(localParm);
                }
            }
            break;
            
        case DLL_PROCESS_DETACH:
            printf("libagent: DLL_PROCESS_DETACH\n");
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }
    return bReturnValue;
}
