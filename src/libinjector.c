// injector_debug.c
#include "windows.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "LoadLibraryR.h"
#include "InjectorNative.h"
#include "InjectParameters.h"

#define WIN_X64
#define BREAK_WITH_ERROR( e ) { printf("[-] %s. Error=%lu\n", e, GetLastError()); break; }

/* Helpers: parse PE header to get Machine and check for ReflectiveLoader export */

/* return IMAGE_FILE_MACHINE_* value or 0 on error */
static WORD get_pe_machine(LPVOID buffer, DWORD length) {
    if (!buffer || length < sizeof(IMAGE_DOS_HEADER)) return 0;
    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)buffer;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;
    if ((DWORD)dos->e_lfanew + sizeof(IMAGE_NT_HEADERS) > length) return 0;
    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((BYTE*)buffer + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return 0;
    return nt->FileHeader.Machine;
}

/* return TRUE if export table contains "ReflectiveLoader" */
static BOOL has_reflective_loader_export(LPVOID buffer, DWORD length) {
    if (!buffer || length < sizeof(IMAGE_DOS_HEADER)) return FALSE;
    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)buffer;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return FALSE;
    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((BYTE*)buffer + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return FALSE;

    DWORD exportRVA = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    DWORD exportSize = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
    if (!exportRVA || exportSize == 0) return FALSE;

    // Need to convert RVA -> raw offset: iterate sections
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(nt);
    WORD numberOfSections = nt->FileHeader.NumberOfSections;
    DWORD exportOffset = 0;
    for (WORD i = 0; i < numberOfSections; ++i) {
        DWORD va = section[i].VirtualAddress;
        DWORD vs = section[i].Misc.VirtualSize;
        DWORD ptr = section[i].PointerToRawData;
        DWORD size = section[i].SizeOfRawData;
        if (exportRVA >= va && exportRVA < va + vs) {
            exportOffset = ptr + (exportRVA - va);
            break;
        }
    }
    if (!exportOffset) return FALSE;
    if ((DWORD)exportOffset + sizeof(IMAGE_EXPORT_DIRECTORY) > length) return FALSE;

    PIMAGE_EXPORT_DIRECTORY exp = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)buffer + exportOffset);
    if (exp->NumberOfNames == 0) return FALSE;

    // locate arrays
    DWORD addrOfNamesRVA = exp->AddressOfNames;
    DWORD addrOfNameOrdinalsRVA = exp->AddressOfNameOrdinals;
    if (!addrOfNamesRVA || !addrOfNameOrdinalsRVA) return FALSE;

    // find raw offsets for these arrays
    DWORD namesOffset = 0;
    DWORD ordinalsOffset = 0;
    for (WORD i = 0; i < numberOfSections; ++i) {
        DWORD va = section[i].VirtualAddress;
        DWORD vs = section[i].Misc.VirtualSize;
        DWORD ptr = section[i].PointerToRawData;
        if (addrOfNamesRVA >= va && addrOfNamesRVA < va + vs) {
            namesOffset = ptr + (addrOfNamesRVA - va);
        }
        if (addrOfNameOrdinalsRVA >= va && addrOfNameOrdinalsRVA < va + vs) {
            ordinalsOffset = ptr + (addrOfNameOrdinalsRVA - va);
        }
    }
    if (!namesOffset || !ordinalsOffset) return FALSE;
    if ((DWORD)namesOffset + exp->NumberOfNames * sizeof(DWORD) > length) return FALSE;
    if ((DWORD)ordinalsOffset + exp->NumberOfNames * sizeof(WORD) > length) return FALSE;

    // iterate names
    for (DWORD i = 0; i < exp->NumberOfNames; ++i) {
        DWORD nameRVA = *(DWORD*)((BYTE*)buffer + namesOffset + i * sizeof(DWORD));
        // convert nameRVA to raw offset
        DWORD nameOffset = 0;
        for (WORD s = 0; s < numberOfSections; ++s) {
            DWORD va = section[s].VirtualAddress;
            DWORD vs = section[s].Misc.VirtualSize;
            DWORD ptr = section[s].PointerToRawData;
            if (nameRVA >= va && nameRVA < va + vs) {
                nameOffset = ptr + (nameRVA - va);
                break;
            }
        }
        if (!nameOffset) continue;
        if ((DWORD)nameOffset >= length) continue;
        char *name = (char*)((BYTE*)buffer + nameOffset);
        // safe compare - ensure string is within buffer
        size_t maxlen = length - nameOffset;
        if (strncasecmp(name, "ReflectiveLoader", maxlen) == 0) return TRUE;
    }
    return FALSE;
}

/* Pretty-print machine */
static const char* machine_to_str(WORD machine) {
    switch(machine) {
        case IMAGE_FILE_MACHINE_I386: return "x86 (IMAGE_FILE_MACHINE_I386)";
        case IMAGE_FILE_MACHINE_AMD64: return "x64 (IMAGE_FILE_MACHINE_AMD64)";
        case IMAGE_FILE_MACHINE_ARM: return "ARM";
        case IMAGE_FILE_MACHINE_ARM64: return "ARM64";
        default: return "Unknown";
    }
}

/* Debug-enhanced inject */
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

    printf("[*] Injector process pointer size = %zu bytes\n", sizeof(void*));

    do {
        if (!params) {
            printf("[-] params is NULL\n");
            BREAK_WITH_ERROR("Invalid parameters");
        }

        /* open DLL file */
        hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
            BREAK_WITH_ERROR("Failed to open DLL file");
		/* Debug helper: print the path being opened and its absolute path */
		char fullPath[MAX_PATH] = {0};
		if (GetFullPathNameA(path, MAX_PATH, fullPath, NULL) != 0) {
			printf("[*] Injector will open DLL path: %s\n", fullPath);
		} else {
			printf("[*] Injector GetFullPathNameA failed: %lu\n", GetLastError());
		}

		/* Before CreateFileA, print which file we are about to open */
		printf("[*] About to open DLL: %s\n", path);

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
        printf("[*] DLL PE machine: %s (0x%04x)\n", machine_to_str(dllMachine), dllMachine);

        /* check if DLL has ReflectiveLoader export */
        BOOL hasRL = has_reflective_loader_export(lpBuffer, dwLength);
        printf("[*] DLL has ReflectiveLoader export: %s\n", hasRL ? "YES" : "NO");

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
            printf("[!] IsWow64Process failed: %lu\n", GetLastError());
        } else {
            printf("[*] Is target process wow64 = %d (TRUE means 32-bit process on 64-bit OS)\n", targetIsWow64);
        }

        /* compare architectures */
#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)
        printf("[*] Injector compiled as: x64\n");
#else
        printf("[*] Injector compiled as: x86\n");
#endif

        if (dllMachine == IMAGE_FILE_MACHINE_AMD64) {
            printf("[*] DLL is x64\n");
#if !defined(_M_X64) && !defined(__x86_64__)
            printf("[-] MISMATCH: injector is 32-bit but DLL is x64. This will fail.\n");
#endif
        } else if (dllMachine == IMAGE_FILE_MACHINE_I386) {
            printf("[*] DLL is x86\n");
#if defined(_M_X64) || defined(__x86_64__)
            printf("[-] MISMATCH: injector is 64-bit but DLL is x86. This will fail.\n");
#endif
        } else {
            printf("[!] Unknown DLL machine type; proceed with caution\n");
        }

        /* allocate remote memory for InjectParameters */
        lpRemoteParam = VirtualAllocEx(hProcess, NULL, sizeof(InjectParameters), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!lpRemoteParam)
            BREAK_WITH_ERROR("VirtualAllocEx for remote param failed");

        printf("[*] remote param addr = %p\n", lpRemoteParam);

        /* write params to remote memory */
        SIZE_T written = 0;
        if (!WriteProcessMemory(hProcess, lpRemoteParam, params, sizeof(InjectParameters), &written) || written != sizeof(InjectParameters))
            BREAK_WITH_ERROR("WriteProcessMemory for remote param failed");

        /* read back some bytes to verify write */
        {
            InjectParameters verify;
            SIZE_T read = 0;
            if (ReadProcessMemory(hProcess, lpRemoteParam, &verify, sizeof(InjectParameters), &read)) {
                printf("[*] Read back remote param, loaderDir=%.*s\n", (int)sizeof(verify.loaderDir), verify.loaderDir);
            } else {
                printf("[-] ReadProcessMemory failed: %lu\n", GetLastError());
            }
        }

        /* call LoadRemoteLibraryR */
        hRemoteThread = LoadRemoteLibraryR(hProcess, lpBuffer, dwLength, lpRemoteParam);
        if (!hRemoteThread) {
            /* library didn't set last error; print hint info we gathered */
            printf("[-] LoadRemoteLibraryR returned NULL. GetLastError=%lu\n", GetLastError());
            printf("[-] Diagnostic summary: dllMachine=0x%04x, hasReflectiveLoader=%d, targetIsWow64=%d\n",
                   dllMachine, hasRL ? 1 : 0, targetIsWow64 ? 1 : 0);
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
