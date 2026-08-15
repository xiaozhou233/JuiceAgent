#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <jni.h>
#include <ReflectiveDLLInjection/LoadLibraryR.h>

#define BREAK_WITH_ERROR(e) { printf("[-] %s. Error=%lu\n", e, GetLastError()); break; }

__declspec(dllexport)
int inject(int pid, const char *path, const char *params){
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

    do {
        /* open DLL file */
        hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
            BREAK_WITH_ERROR("Failed to open DLL file");

        /* get file size */
        dwLength = GetFileSize(hFile, NULL);
        if (dwLength == INVALID_FILE_SIZE || dwLength == 0)
            BREAK_WITH_ERROR("Failed to get DLL file size");

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

        /* write params to remote memory */
        SIZE_T written = 0;
        if (params) {
            lpRemoteParam = VirtualAllocEx(hProcess, NULL, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (!lpRemoteParam)
                BREAK_WITH_ERROR("VirtualAllocEx for remote param failed");

            SIZE_T param_len = strlen(params) + 1;
            if (param_len > MAX_PATH)
                param_len = MAX_PATH;

            if (!WriteProcessMemory(hProcess, lpRemoteParam, params, param_len, &written) || written != param_len)
                BREAK_WITH_ERROR("WriteProcessMemory for remote param failed");
        }

        /* call LoadRemoteLibraryR */
        hRemoteThread = LoadRemoteLibraryR(hProcess, lpBuffer, dwLength, lpRemoteParam);
        if (!hRemoteThread)
            BREAK_WITH_ERROR("LoadRemoteLibraryR returned NULL");

        printf("[+] DLL injected: '%s' into PID %lu\n", path, dwProcessId);
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

static jboolean jni_inject(JNIEnv* env, jint pid, jstring path, const char* config_dir) {
    const char* dll_path = (*env)->GetStringUTFChars(env, path, NULL);
    if (!dll_path)
        return JNI_FALSE;

    char params[MAX_PATH] = {0};
    const char* inject_params = NULL;
    if (config_dir) {
        strncpy(params, config_dir, MAX_PATH - 1);
        params[MAX_PATH - 1] = '\0';
        inject_params = params;
    }

    int ret = inject(pid, dll_path, inject_params);

    (*env)->ReleaseStringUTFChars(env, path, dll_path);
    return (ret == 0) ? JNI_TRUE : JNI_FALSE;
}

/*
* JNI Function: inject(int pid, String path)
*/
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceagent_injector_InjectorNative_inject__ILjava_lang_String_2
  (JNIEnv *env, jobject obj, jint pid, jstring path) {
    (void)obj;
    return jni_inject(env, pid, path, NULL);
}

/*
* JNI Function: inject(int pid, String path, String configDir)
* configDir: path of config file (toml)
*/
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceagent_injector_InjectorNative_inject__ILjava_lang_String_2Ljava_lang_String_2
  (JNIEnv *env, jobject obj, jint pid, jstring path, jstring configDir) {
    (void)obj;
    const char* config_dir = (*env)->GetStringUTFChars(env, configDir, NULL);
    jboolean result = jni_inject(env, pid, path, config_dir);
    (*env)->ReleaseStringUTFChars(env, configDir, config_dir);
    return result;
}

/* ================ FindWindowsByTitle ================= */
#define MAX_RESULTS 128

typedef struct {
    wchar_t title[256];
    DWORD pid;
} WindowData;

static wchar_t g_keyword[256];
static WindowData g_results[MAX_RESULTS];
static int g_count = 0;

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    (void)lParam;
    wchar_t title[256];
    GetWindowTextW(hwnd, title, 256);
    if (wcslen(title) == 0) return TRUE;

    if (wcsstr(title, g_keyword)) {
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);

        if (g_count < MAX_RESULTS && pid) {
            wcsncpy(g_results[g_count].title, title, 255);
            g_results[g_count].title[255] = L'\0';
            g_results[g_count].pid = pid;
            g_count++;
        }
    }
    return TRUE;
}

JNIEXPORT jobjectArray JNICALL Java_cn_xiaozhou233_juiceagent_injector_InjectorNative_findWindowsByTitle
  (JNIEnv *env, jclass clazz, jstring keyword) {
    (void)clazz;
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
/* ====================================================== */
