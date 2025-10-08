// injector_debug.c
#include "InjectorUtils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "LoadLibraryR.h"
#include "InjectorNative.h"
#include "InjectParameters.h"
#include "log.c"

#define WIN_X64
#define BREAK_WITH_ERROR( e ) { log_error("[-] %s. Error=%l", e, GetLastError()); break; }

int inject(int pid, char *path, InjectParameters *params){
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

    log_set_level(LOG_DEBUG);
    log_is_log_filename(false);
    log_is_log_line(false);
    log_is_log_time(false);

    log_debug("Injector process pointer size = %zu bytes", sizeof(void*));
    

    do {
        if (!params) {
            log_error("Invalid parameters");
            BREAK_WITH_ERROR("Invalid parameters");
        }

        /* open DLL file */
        hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
            BREAK_WITH_ERROR("Failed to open DLL file");
		/* Debug helper: print the path being opened and its absolute path */
		char fullPath[MAX_PATH] = {0};
		if (GetFullPathNameA(path, MAX_PATH, fullPath, NULL) != 0) {
            log_debug("Injector GetFullPathNameA success: %s", fullPath);
		} else {
            log_error("[*] Injector GetFullPathNameA failed: %lu", GetLastError());
		}

		/* Before CreateFileA, print which file we are about to open */
        log_debug("[*] About to open DLL: %s", path);

		/* after ReadFile (or after loading into lpBuffer), verify exports again on the buffer we actually read */

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

        /* print machine type of DLL buffer */
        WORD dllMachine = get_pe_machine(lpBuffer, dwLength);
        log_debug("[*] DLL PE machine: %s (0x%04x)", machine_to_str(dllMachine), dllMachine);

        /* check if DLL has ReflectiveLoader export */
        BOOL hasRL = has_reflective_loader_export(lpBuffer, dwLength);
        log_debug("[*] DLL has ReflectiveLoader export: %s", hasRL ? "YES" : "NO");

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
            log_error("IsWow64Process failed");
        } else {
            log_info("Is target process wow64 = %d (TRUE means 32-bit process on 64-bit OS)", targetIsWow64);
        }

        /* compare architectures */
#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)
        log_info("Injector compiled as: x64");
#else
        log_info("Injector compiled as: x86");
#endif

        if (dllMachine == IMAGE_FILE_MACHINE_AMD64) {
            log_info("[*] DLL is x64");
#if !defined(_M_X64) && !defined(__x86_64__)
            log_warn("[-] MISMATCH: injector is 32-bit but DLL is x64. This will fail.");
#endif
        } else if (dllMachine == IMAGE_FILE_MACHINE_I386) {
            log_info("[*] DLL is x86");
#if defined(_M_X64) || defined(__x86_64__)
            log_warn("[-] MISMATCH: injector is 64-bit but DLL is x86. This will fail.");
#endif
        } else {
            log_error("Unknown DLL machine type; proceed with caution");
        }

        /* allocate remote memory for InjectParameters */
        lpRemoteParam = VirtualAllocEx(hProcess, NULL, sizeof(InjectParameters), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!lpRemoteParam)
            BREAK_WITH_ERROR("VirtualAllocEx for remote param failed");

        log_debug("remote param addr = %p", lpRemoteParam);

        /* write params to remote memory */
        SIZE_T written = 0;
        if (!WriteProcessMemory(hProcess, lpRemoteParam, params, sizeof(InjectParameters), &written) || written != sizeof(InjectParameters))
            BREAK_WITH_ERROR("WriteProcessMemory for remote param failed");

        /* read back some bytes to verify write */
        {
            InjectParameters verify;
            SIZE_T read = 0;
            if (ReadProcessMemory(hProcess, lpRemoteParam, &verify, sizeof(InjectParameters), &read)) {
                log_debug("[*] Read back remote param, loaderDir=%.*s", (int)sizeof(verify.loaderDir), verify.loaderDir);
            } else {
                log_error("[-] ReadProcessMemory failed: %lu", GetLastError());
            }
        }

        /* call LoadRemoteLibraryR */
        hRemoteThread = LoadRemoteLibraryR(hProcess, lpBuffer, dwLength, lpRemoteParam);
        if (!hRemoteThread) {
            /* library didn't set last error; print hint info we gathered */
            log_error("[-] LoadRemoteLibraryR returned NULL. GetLastError=%lu", GetLastError());
            log_error("[-] Diagnostic summary: dllMachine=0x%04x, hasReflectiveLoader=%d, targetIsWow64=%d",
                   dllMachine, hasRL ? 1 : 0, targetIsWow64 ? 1 : 0);
            BREAK_WITH_ERROR("LoadRemoteLibraryR returned NULL");
        }

        log_info("[+] DLL injected: '%s' into PID %d, remote param at %p", path, dwProcessId, lpRemoteParam);
        /* wait for reflective loader thread to complete */
        WaitForSingleObject(hRemoteThread, INFINITE);

        /* get remote thread exit code */
        DWORD exitCode = 0;
        if (GetExitCodeThread(hRemoteThread, &exitCode))
            log_info("[+] Remote thread exit code: %lu", exitCode);

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

/* wrapper using HOME */
int inject_pid(int pid){
    const char* home = getenv("HOME");
    if (!home) return -1;

    char dir[MAX_PATH];
    snprintf(dir, sizeof(dir), "%s\\JuiceAgent\\", home);

    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\libagent.dll", dir);

    InjectParameters params;
    memset(&params, 0, sizeof(params));
    strncpy(params.loaderDir, dir, sizeof(params.loaderDir)-1);
    params.loaderDir[sizeof(params.loaderDir)-1] = '\0';

    return inject(pid, path, &params);
}

/* JNI interface */
JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_orangex_injector_InjectorNative_inject
  (JNIEnv *env, jobject obj, jint pid, jstring path, jstring loaderDir) {

    const char* nativePath = (*env)->GetStringUTFChars(env, path, 0);
    const char* nativeLoaderDir = (*env)->GetStringUTFChars(env, loaderDir, 0);

    InjectParameters params;
    memset(&params, 0, sizeof(params));
    strncpy(params.loaderDir, nativeLoaderDir, sizeof(params.loaderDir)-1);
    params.loaderDir[sizeof(params.loaderDir)-1] = '\0';

    int ret = inject(pid, (char*)nativePath, &params);

    (*env)->ReleaseStringUTFChars(env, path, nativePath);
    (*env)->ReleaseStringUTFChars(env, loaderDir, nativeLoaderDir);

    return (ret == 0) ? JNI_TRUE : JNI_FALSE;
}
