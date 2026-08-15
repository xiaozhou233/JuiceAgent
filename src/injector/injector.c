#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define MAX_PATH_LEN 4096
#define MAX_INPUT_LEN 128

static void execute_jps(void)
{
    FILE* pipe = _popen("jps -l", "r");
    if (!pipe)
    {
        printf("[-] Failed to start 'jps -l'.\n");
        return;
    }

    char buffer[MAX_INPUT_LEN];
    while (fgets(buffer, sizeof(buffer), pipe))
    {
        fputs(buffer, stdout);
    }
    _pclose(pipe);
}

static void copy_arg(char* dst, size_t dst_size, const char* src)
{
    int written = snprintf(dst, dst_size, "%s", src ? src : "");
    if (written < 0 || (size_t)written >= dst_size)
    {
        printf("[-] Argument truncated to fit buffer.\n");
    }
}

static int run(int argc, char** argv)
{
    int pid = 0;
    char dll_path[MAX_PATH_LEN] = {0};
    char injector_lib_path[MAX_PATH_LEN] = {0};
    char cwd[MAX_PATH_LEN] = {0};

    if (argc >= 3)
    {
        pid = atoi(argv[1]);
        copy_arg(dll_path, sizeof(dll_path), argv[2]);
        if (argc >= 4)
        {
            copy_arg(injector_lib_path, sizeof(injector_lib_path), argv[3]);
        }
    }
    else
    {
        execute_jps();
        printf("Input PID: ");

        char line[MAX_INPUT_LEN] = {0};
        if (!fgets(line, sizeof(line), stdin))
        {
            printf("[-] Failed to read PID.\n");
            return 1;
        }
        pid = atoi(line);

        if (!GetCurrentDirectoryA(sizeof(cwd), cwd))
        {
            printf("[-] GetCurrentDirectory failed, using '.'\n");
            copy_arg(cwd, sizeof(cwd), ".");
        }
        snprintf(dll_path, sizeof(dll_path), "%s\\libloader.dll", cwd);
        printf("[*] Using DLL path: %s\n", dll_path);
    }

    if (pid <= 0)
    {
        printf("[-] Invalid or missing PID.\n");
        return 1;
    }

    HMODULE hLib = NULL;
    if (injector_lib_path[0] != '\0')
    {
        hLib = LoadLibraryA(injector_lib_path);
        if (!hLib)
        {
            printf("[-] Failed to load injector library: %s (Error=%lu)\n",
                   injector_lib_path, GetLastError());
            return 2;
        }
    }
    else
    {
        hLib = LoadLibraryA("libinject.dll");
        if (!hLib)
        {
            printf("[-] Failed to load libinject.dll by name (Error=%lu).\n",
                   GetLastError());
            printf("    Provide a full path as the 3rd argument if needed.\n");
            return 2;
        }
    }
    printf("[+] Loaded injector library.\n");

    typedef int (__cdecl* inject_fn_t)(int, const char*, void*);

    FARPROC proc = GetProcAddress(hLib, "inject");
    if (!proc)
    {
        proc = GetProcAddress(hLib, "Inject");
    }
    if (!proc)
    {
        printf("[-] GetProcAddress failed for 'inject' (Error=%lu).\n",
               GetLastError());
        printf("    Make sure the injector exports C symbol 'inject'.\n");
        FreeLibrary(hLib);
        return 3;
    }

    inject_fn_t inject = (inject_fn_t)proc;

    printf("[*] Calling inject(pid=%d, dllPath=\"%s\", currentDir=\"%s\")\n",
           pid, dll_path, cwd);
    int ret = inject(pid, dll_path, cwd);

    if (ret == 0)
    {
        printf("[+] inject returned success (0)\n");
    }
    else
    {
        printf("[-] inject returned failure (%d)\n", ret);
    }

    FreeLibrary(hLib);
    return (ret == 0) ? 0 : 4;
}

int main(int argc, char** argv)
{
    int ret = run(argc, argv);

    printf("\nPress any key to exit...\n");
    getchar();

    return ret;
}
