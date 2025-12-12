#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PATH_LEN 4096

void ExecuteJps(){
    // Execute "jps -l" and print the output
    char command[MAX_PATH] = "jps -l";
    FILE* pipe = _popen(command, "r");
    if (!pipe) {
        printf("Error opening pipe!\n");
        return;
    }
    char buffer[128];
    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != NULL)
            printf("%s", buffer);
    }
    _pclose(pipe);
}

int main(int argc, char **argv) {
    int pid = 0;
    char dll_path[MAX_PATH_LEN] = {0};
    char injector_lib_path[MAX_PATH_LEN] = {0};
    char cwd[MAX_PATH_LEN] = {0};

    /* If user provided pid and dll path as arguments, use them. */
    if (argc >= 3) {
        pid = atoi(argv[1]);
        strncpy(dll_path, argv[2], MAX_PATH_LEN - 1);
        if (argc >= 4) {
            strncpy(injector_lib_path, argv[3], MAX_PATH_LEN - 1);
        }
    } else {
        /* Show jps output to help user find JVM PID */
        ExecuteJps();
        printf("Input PID: ");
        if (scanf("%d", &pid) != 1 || pid <= 0) {
            printf("Invalid PID input.\n");
            return 1;
        }

        /* Default dll_path = currentdir\\libloader.dll */
        if (!GetCurrentDirectoryA(MAX_PATH_LEN, cwd)) {
            printf("Warning: GetCurrentDirectory failed, using '.'\n");
            strncpy(cwd, ".", MAX_PATH_LEN - 1);
        }
        snprintf(dll_path, MAX_PATH_LEN, "%s\\libagent.dll", cwd);
        printf("Using DLL path: %s\n", dll_path);
    }

    if (pid <= 0) {
        printf("Invalid or missing PID.\n");
        return 1;
    }

    /* Load injector library */
    HMODULE hLib = NULL;
    if (injector_lib_path[0] != '\0') {
        hLib = LoadLibraryA(injector_lib_path);
        if (!hLib) {
            printf("[-] Failed to load injector library: %s (Error=%lu)\n", injector_lib_path, GetLastError());
            return 2;
        }
    } else {
        /* Try to load by name from current dir / PATH */
        hLib = LoadLibraryA("libinject.dll");
        if (!hLib) {
            printf("[-] Failed to load libinjector.dll by name (Error=%lu).\n", GetLastError());
            printf("    Provide full path as the 3rd argument if needed.\n");
            return 2;
        }
    }
    printf("[+] Loaded injector library.\n");

    /* Define function pointer type (cdecl) - match the DLL export */
    typedef int (__cdecl *inject_fn_t)(int, const char*, void*);

    /* GetProcAddress for 'inject' */
    FARPROC proc = GetProcAddress(hLib, "inject");
    if (!proc) {
        /* Try common alternative name */
        proc = GetProcAddress(hLib, "Inject");
    }
    if (!proc) {
        printf("[-] GetProcAddress failed for 'inject' (Error=%lu). Make sure your injector exports C symbol 'inject'.\n", GetLastError());
        FreeLibrary(hLib);
        return 3;
    }

    inject_fn_t inject = (inject_fn_t)proc;

    printf("[*] Calling inject(pid=%d, dllPath=\"%s\", currentDir=\"%s\")\n", pid, dll_path, cwd);
    int ret = inject(pid, dll_path, cwd);

    if (ret == 0) {
        printf("[+] inject returned success (0)\n");
    } else {
        printf("[-] inject returned failure (%d)\n", ret);
    }

    FreeLibrary(hLib);
    return (ret == 0) ? 0 : 4;
}
