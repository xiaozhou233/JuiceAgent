#include <JuiceAgent.h>
#include <Logger.hpp>
#include <windows.h>
#include <InjectorUtils.h>
#include <jni.h>
#include <string.h>

#define BREAK_WITH_ERROR( e ) { PLOGE.printf("[-] %s. Error=%lu", e, GetLastError()); break; }

/*
 * Standard DLL injection:
 *  - write DLL path string into remote process memory
 *  - CreateRemoteThread with LoadLibraryA(remotePath)
 *  - wait, get exit code, clean up
 */

extern "C" __declspec(dllexport)
int inject(int pid, const char *path, InjectParams *params) {
    HANDLE hFile          = NULL;
    HANDLE hRemoteThread  = NULL;
    HANDLE hProcess       = NULL;
    HANDLE hToken         = NULL;
    LPVOID lpBuffer       = NULL;        // local file buffer (optional, used for PE inspection)
    LPVOID lpRemoteParam  = NULL;        // REMOTE memory for InjectParams (not used in standard injection)
    LPVOID lpRemotePath   = NULL;        // REMOTE memory for path string (used)
    DWORD dwLength        = 0;
    DWORD dwBytesRead     = 0;
    DWORD dwProcessId     = (DWORD)pid;
    TOKEN_PRIVILEGES priv = {0};
    BOOL bSuccess         = FALSE;

    InitLogger();

    PLOGI.printf("[*] libinjector Version %d.%d Build %d", PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_BUILD_NUMBER);
    PLOGD << "[*] InjectParams pointer: " << &params;

    if (!path) {
        PLOGE << "[!] Invalid path";
        return -1;
    }

    do {
        /* optional: open DLL file to inspect PE info (kept for logging) */
        hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            PLOGW.printf("[!] Cannot open DLL for inspection: %s, Error=%lu -- will still try standard injection", path, GetLastError());
            hFile = NULL; // continue without file buffer
        } else {
            /* get file size (use GetFileSize) */
            dwLength = GetFileSize(hFile, NULL);
            if (dwLength != INVALID_FILE_SIZE && dwLength > 0) {
                lpBuffer = HeapAlloc(GetProcessHeap(), 0, dwLength);
                if (lpBuffer) {
                    if (!ReadFile(hFile, lpBuffer, dwLength, &dwBytesRead, NULL) || dwBytesRead != dwLength) {
                        PLOGW.printf("[!] ReadFile incomplete or failed: %lu", GetLastError());
                        HeapFree(GetProcessHeap(), 0, lpBuffer);
                        lpBuffer = NULL;
                    } else {
                        WORD dllMachine = get_pe_machine(lpBuffer, dwLength);
                        PLOGD.printf("[*] DLL PE machine: %s (0x%04x)", machine_to_str(dllMachine), dllMachine);
                        BOOL hasRL = has_reflective_loader_export(lpBuffer, dwLength);
                        PLOGD.printf("[*] DLL has ReflectiveLoader export: %s", hasRL ? "YES" : "NO");
                    }
                } else {
                    PLOGW.printf("[!] HeapAlloc for file buffer failed: %lu", GetLastError());
                }
            } else {
                PLOGW.printf("[!] GetFileSize returned invalid or zero size: %lu", GetLastError());
            }
        }

        /* try to enable SeDebugPrivilege (best-effort) */
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
        if (!hProcess) {
            BREAK_WITH_ERROR("Failed to open target process");
        }

        /* detect target architecture (logging only) */
        BOOL targetIsWow64 = FALSE;
        if (!IsWow64Process(hProcess, &targetIsWow64)) {
            PLOGE.printf("IsWow64Process failed: %lu", GetLastError());
        } else {
            PLOGI.printf("Is target process wow64 = %d (TRUE means 32-bit process on 64-bit OS)", targetIsWow64);
        }

#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)
        PLOGI.printf("Injector compiled as: x64");
#else
        PLOGI.printf("Injector compiled as: x86");
#endif

        /* allocate remote memory for DLL path string */
        size_t pathLen = strlen(path) + 1;
        lpRemotePath = VirtualAllocEx(hProcess, NULL, pathLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!lpRemotePath) {
            BREAK_WITH_ERROR("VirtualAllocEx for remote path failed");
        }

        PLOGD.printf("remote path addr = %p", lpRemotePath);

        /* write DLL path into remote process */
        SIZE_T written = 0;
        if (!WriteProcessMemory(hProcess, lpRemotePath, path, pathLen, &written) || written != pathLen) {
            BREAK_WITH_ERROR("WriteProcessMemory for remote path failed");
        }

        /* create remote thread calling LoadLibraryA(remotePath) */
        DWORD dwThreadId = 0;
        hRemoteThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, lpRemotePath, 0, &dwThreadId);
        if (!hRemoteThread) {
            BREAK_WITH_ERROR("CreateRemoteThread failed");
        }

        PLOGI.printf("[+] DLL injection thread created for '%s' into PID %d (threadId=%lu)", path, dwProcessId, dwThreadId);

        /* wait for thread to finish */
        WaitForSingleObject(hRemoteThread, INFINITE);

        /* get remote thread exit code (module handle or 0) */
        DWORD exitCode = 0;
        if (GetExitCodeThread(hRemoteThread, &exitCode)) {
            if (exitCode == 0) {
                PLOGW.printf("[-] Remote LoadLibraryA returned NULL (failed to load).");
            } else {
                PLOGI.printf("[+] Remote LoadLibraryA returned module handle: 0x%08lx", exitCode);
            }
        } else {
            PLOGW.printf("[-] GetExitCodeThread failed: %lu", GetLastError());
        }

        bSuccess = TRUE;

        /* cleanup remote thread handle */
        CloseHandle(hRemoteThread);
        hRemoteThread = NULL;

        /* free remote path memory */
        if (lpRemotePath) {
            VirtualFreeEx(hProcess, lpRemotePath, 0, MEM_RELEASE);
            lpRemotePath = NULL;
        }

    } while (0);

    /* cleanup local resources */
    if (lpBuffer) {
        HeapFree(GetProcessHeap(), 0, lpBuffer);
        lpBuffer = NULL;
    }
    if (hFile && hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
        hFile = NULL;
    }
    if (hProcess) {
        CloseHandle(hProcess);
        hProcess = NULL;
    }

    return bSuccess ? 0 : -1;
}

/* JNI wrappers (unchanged except they call the above inject) */

/*
* JNI Function: inject(int pid, String path)
*/
extern "C" __declspec(dllexport)
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceagent_injector_InjectorNative_inject__ILjava_lang_String_2
  (JNIEnv *env, jobject obj, jint pid, jstring path) {
    const char* InjectionDLL = env->GetStringUTFChars(path, NULL);
    int ret = inject(pid, InjectionDLL, NULL);
    env->ReleaseStringUTFChars(path, InjectionDLL);
    return (ret == 0) ? JNI_TRUE : JNI_FALSE;
}

/*
* JNI Function: inject(int pid, String path, String configDir)
*/
extern "C" __declspec(dllexport)
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_juiceagent_injector_InjectorNative_inject__ILjava_lang_String_2Ljava_lang_String_2
  (JNIEnv *env, jobject obj, jint pid , jstring path, jstring configDir){
    const char* InjectionDLL = env->GetStringUTFChars(path, NULL);
    const char* ConfigDir = env->GetStringUTFChars(configDir, NULL);

    /* NOTE: Standard LoadLibrary injection cannot directly pass InjectParams.
       If your DLL needs ConfigDir, it should read it from a known location (e.g. file, registry),
       or you can implement an exported init function inside the DLL which you call later. */

    int ret = inject(pid, InjectionDLL, NULL);

    env->ReleaseStringUTFChars(path, InjectionDLL);
    env->ReleaseStringUTFChars(configDir, ConfigDir);

    return (ret == 0) ? JNI_TRUE : JNI_FALSE;
}

/* Find windows by title implementation (unchanged) */

#define MAX_RESULTS 128

typedef struct {
    wchar_t title[256];
    DWORD pid;
} WindowData;

static wchar_t g_keyword[256];
static WindowData g_results[MAX_RESULTS];
static int g_count = 0;

extern "C" __declspec(dllexport)
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

extern "C" __declspec(dllexport)
JNIEXPORT jobjectArray JNICALL Java_cn_xiaozhou233_juiceagent_injector_InjectorNative_findWindowsByTitle
  (JNIEnv *env, jclass clazz, jstring keyword) {

    const jchar *input = env->GetStringChars(keyword, NULL);
    wcsncpy(g_keyword, (const wchar_t *)input, 255);
    g_keyword[255] = L'\0';
    env->ReleaseStringChars(keyword, input);

    g_count = 0;
    EnumWindows(EnumWindowsProc, 0);

    jclass infoClass = env->FindClass("cn/xiaozhou233/juiceagent/injector/InjectorNative$WindowInfo");
    if (!infoClass) return NULL;

    jmethodID ctor = env->GetMethodID(infoClass, "<init>", "(Ljava/lang/String;I)V");
    if (!ctor) return NULL;

    jobjectArray array = env->NewObjectArray(g_count, infoClass, NULL);

    for (int i = 0; i < g_count; i++) {
        jstring title = env->NewString((jchar*)g_results[i].title, wcslen(g_results[i].title));
        jobject obj = env->NewObject(infoClass, ctor, title, (jint)g_results[i].pid);
        env->SetObjectArrayElement(array, i, obj);
    }

    return array;
}
