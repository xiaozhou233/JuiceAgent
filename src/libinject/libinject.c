#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jni.h>
#include <ReflectiveDLLInjection/LoadLibraryR.h>

#define WIN_X64
#define BREAK_WITH_ERROR( e ) { printf("[-] %s. Error=%l", e, GetLastError()); break; }
#define MAX_PATH 260

__declspec(dllexport)
int inject(int pid, char *path, char *params){
    HANDLE hFile          = NULL;
    HANDLE hRemoteThread  = NULL;
    HANDLE hProcess       = NULL;
    HANDLE hToken         = NULL;
    LPVOID lpBuffer       = NULL;
    LPVOID lpRemoteParam  = NULL;
    DWORD dwLength        = 0;
    DWORD dwBytesRead     = 0;
    DWORD dwProcessId     = (DWORD)pid;
    TOKEN_PRIVILEGES priv = {0};
    BOOL bSuccess         = FALSE;

    printf("Injector process pointer size = %zu bytes\n", sizeof(void*));
    
    do {
        if (!params) {
            printf("[!] No parameters provided\n");
        }

        /* open DLL file */
        hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
            BREAK_WITH_ERROR("Failed to open DLL file");
		/* Debug helper: print the path being opened and its absolute path */
		char fullPath[MAX_PATH] = {0};
		if (GetFullPathNameA(path, MAX_PATH, fullPath, NULL) != 0) {
            printf("Injector GetFullPathNameA success: %s\n", fullPath);
		} else {
            printf("[*] Injector GetFullPathNameA failed: %lu\n", GetLastError());
		}

		/* Before CreateFileA, print which file we are about to open */
        printf("[*] About to open DLL: %s\n", path);

		/* after ReadFile (or after loading into lpBuffer), verify exports again on the buffer we actually read */

        /* get file size */
        dwLength = GetFileSize(hFile, NULL);
        if (dwLength == INVALID_FILE_SIZE || dwLength == 0)
            BREAK_WITH_ERROR("Failed to get DLL file size\n");

        /* alloc local buffer and read file */
        lpBuffer = HeapAlloc(GetProcessHeap(), 0, dwLength);
        if (!lpBuffer)
            BREAK_WITH_ERROR("HeapAlloc failed");

        if (!ReadFile(hFile, lpBuffer, dwLength, &dwBytesRead, NULL) || dwBytesRead != dwLength)
            BREAK_WITH_ERROR("ReadFile failed or incomplete");

        /* enable SeDebugPrivilege (best-effort) */
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
            priv.PrivilegeCount = 1;
            priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &priv.Privileges[0].Luid))
                AdjustTokenPrivileges(hToken, FALSE, &priv, 0, NULL, NULL);
            CloseHandle(hToken);
            hToken = NULL;
        }

        /* open target process */
        hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, dwProcessId);
        if (!hProcess)
            BREAK_WITH_ERROR("Failed to open target process");

        /* detect target architecture */
        BOOL targetIsWow64 = FALSE;
        if (!IsWow64Process(hProcess, &targetIsWow64)) {
            printf("IsWow64Process failed\n");
        } else {
            printf("Is target process wow64 = %d (TRUE means 32-bit process on 64-bit OS)\n", targetIsWow64);
        }


        /* allocate remote memory for InjectParameters */
        lpRemoteParam = VirtualAllocEx(hProcess, NULL, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!lpRemoteParam)
            BREAK_WITH_ERROR("VirtualAllocEx for remote param failed\n");

        printf("remote param addr = %p\n", lpRemoteParam);

        /* write params to remote memory */
        SIZE_T written = 0;
        if (params) {
            if (!WriteProcessMemory(hProcess, lpRemoteParam, params, MAX_PATH, &written) || written != MAX_PATH)
            BREAK_WITH_ERROR("WriteProcessMemory for remote param failed\n");
        } else {
            printf("No parameters provided; using null pointer\n");
            lpRemoteParam = NULL;
        }

        /* read back some bytes to verify write */
        {
            char verify[MAX_PATH] = {0};
            SIZE_T read = 0;

            if (lpRemoteParam) {
                if (ReadProcessMemory(hProcess, lpRemoteParam, verify, MAX_PATH, &read)) {
                    printf("[*] Read back remote param, ConfigDir=%s\n", verify);
                } else {
                    printf("[-] ReadProcessMemory failed: %lu\n", GetLastError());
                }
            } else {
                printf("[*] lpRemoteParam == NULL, skip verify\n");
            }
        }

        /* call LoadRemoteLibraryR */
        hRemoteThread = LoadRemoteLibraryR(hProcess, lpBuffer, dwLength, lpRemoteParam);
        if (!hRemoteThread) {
            /* library didn't set last error; print hint info we gathered */
            printf("[-] LoadRemoteLibraryR returned NULL. GetLastError=%lu\n", GetLastError());
            BREAK_WITH_ERROR("LoadRemoteLibraryR returned NULL");
        }

        printf("[+] DLL injected: '%s' into PID %d, remote param at %p\n", path, dwProcessId, lpRemoteParam);
        /* wait for reflective loader thread to complete */
        WaitForSingleObject(hRemoteThread, INFINITE);

        /* get remote thread exit code */
        DWORD exitCode = 0;
        if (GetExitCodeThread(hRemoteThread, &exitCode))
            printf("[+] Remote thread exit code: %lu\n", exitCode);

        bSuccess = TRUE;

        CloseHandle(hRemoteThread);
        hRemoteThread = NULL;

    } while (0);

    /* cleanup */
    if (lpBuffer) {
        HeapFree(GetProcessHeap(), 0, lpBuffer);
        lpBuffer = NULL;
    }
    if (hFile && hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
        hFile = NULL;
    }
    if (hProcess) {
        /* do not free remote param here unless you know DLL copied it */
        CloseHandle(hProcess);
        hProcess = NULL;
    }

    return bSuccess ? 0 : -1;
}

/*
* JNI Function: inject(int pid)
* Note: 
*/
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceagent_injector_InjectorNative_inject__ILjava_lang_String_2
  (JNIEnv *env, jobject obj, jint pid, jstring path) {
    // Injection Path
    const char* InjectionDLL = (*env)->GetStringUTFChars(env, path, NULL);

    int ret = inject(pid, (char*)InjectionDLL, NULL);
    (*env)->ReleaseStringUTFChars(env, path, InjectionDLL);
    return (ret == 0) ? JNI_TRUE : JNI_FALSE;
}


/*
* JNI Function: inject(int pid, String path)
* Param pid: target process id
* Param path: path of inject dll
* Param configDir: path of config file (toml)
*/
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceagent_injector_InjectorNative_inject__ILjava_lang_String_2Ljava_lang_String_2
  (JNIEnv *env, jobject obj, jint pid , jstring path, jstring configDir){

    const char* InjectionDLL = (*env)->GetStringUTFChars(env, path, NULL);
    const char* ConfigDir = (*env)->GetStringUTFChars(env, configDir, NULL);

    char params[MAX_PATH] = {0};    /* use a real buffer */
    strncpy(params, ConfigDir ? ConfigDir : "", MAX_PATH - 1);

    int ret = inject(pid, (char*)InjectionDLL, params);

    (*env)->ReleaseStringUTFChars(env, path, InjectionDLL);
    (*env)->ReleaseStringUTFChars(env, configDir, ConfigDir);

    return (ret == 0) ? JNI_TRUE : JNI_FALSE;
}

/// ================ FindWindowsByTitle  =================
#define MAX_RESULTS 128

typedef struct {
    wchar_t title[256];
    DWORD pid;
} WindowData;

static wchar_t g_keyword[256];
static WindowData g_results[MAX_RESULTS];
static int g_count = 0;


BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    wchar_t title[256];
    GetWindowTextW(hwnd, title, 256);
    if (wcslen(title) == 0) return TRUE;

    if (wcsstr(title, g_keyword)) {
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);

        if (g_count < MAX_RESULTS && pid) {
            wcsncpy(g_results[g_count].title, title, 255);
            g_results[g_count].pid = pid;
            g_count++;
        }
    }
    return TRUE;
}

JNIEXPORT jobjectArray JNICALL Java_cn_xiaozhou233_juiceagent_injector_InjectorNative_findWindowsByTitle
  (JNIEnv *env, jclass clazz, jstring keyword) {

    const jchar *input = (*env)->GetStringChars(env, keyword, NULL);
    wcsncpy(g_keyword, (const wchar_t *)input, 255);
    g_keyword[255] = L'\0';
    (*env)->ReleaseStringChars(env, keyword, input);

    g_count = 0;
    EnumWindows(EnumWindowsProc, 0);

    jclass infoClass = (*env)->FindClass(env, "cn/xiaozhou233/juiceagent/injector/InjectorNative$WindowInfo");
    if (!infoClass) return NULL;

    jmethodID ctor = (*env)->GetMethodID(env, infoClass, "<init>", "(Ljava/lang/String;I)V");
    if (!ctor) return NULL;

    jobjectArray array = (*env)->NewObjectArray(env, g_count, infoClass, NULL);

    for (int i = 0; i < g_count; i++) {
        jstring title = (*env)->NewString(env, (jchar*)g_results[i].title, wcslen(g_results[i].title));
        jobject obj = (*env)->NewObject(env, infoClass, ctor, title, (jint)g_results[i].pid);
        (*env)->SetObjectArrayElement(env, array, i, obj);
    }

    return array;
}
/// ===============================================================================