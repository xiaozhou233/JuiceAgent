// injector_client.cpp
// Console injector client - uses main() for maximum compatibility.
// Usage:
//    injector_client.exe <pid> <dll_path_to_inject> [injector_lib_path]
//
// Notes:
//  - injector library must export a C symbol: extern "C" __declspec(dllexport) int inject(int, const char*, void*);
//  - This client calls inject(pid, dllPath, NULL).

#include <windows.h>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <pid> <dll_path_to_inject> [injector_lib_path]\n";
        return 1;
    }

    // parse pid
    int pid = atoi(argv[1]);
    if (pid <= 0) {
        std::cerr << "Invalid PID: " << argv[1] << "\n";
        return 1;
    }

    // dll path (UTF-8 or ANSI depending on environment)
    const char* dllPath = argv[2];
    std::string dllPathStr = dllPath;

    // load injector library
    HMODULE hLib = NULL;
    if (argc >= 4) {
        const char* libPath = argv[3];
        hLib = LoadLibraryA(libPath);
        if (!hLib) {
            std::cerr << "[-] Failed to load injector library: " << libPath << ", Error=" << GetLastError() << "\n";
            return 2;
        }
    } else {
        // try to load from current dir / PATH by name
        hLib = LoadLibraryA("libinjector.dll");
        if (!hLib) {
            std::cerr << "[-] Failed to load libinjector.dll by name. Error=" << GetLastError() << "\n";
            std::cerr << "    Provide full path as the 3rd argument if needed.\n";
            return 2;
        }
    }

    std::cout << "[+] Loaded injector library\n";

    // function pointer type: adjust calling convention if your DLL uses different one.
    typedef int (__cdecl *inject_fn_t)(int, const char*, void*);

    FARPROC p = GetProcAddress(hLib, "inject");
    if (!p) {
        // try some common alternatives
        p = GetProcAddress(hLib, "Inject");
    }
    if (!p) {
        std::cerr << "[-] GetProcAddress failed for 'inject'. Make sure your library exports 'inject' as C symbol.\n";
        FreeLibrary(hLib);
        return 3;
    }

    inject_fn_t inject = reinterpret_cast<inject_fn_t>(p);

    std::cout << "[*] Calling inject(pid, dllPath, NULL)\n";
    int result = inject(pid, dllPathStr.c_str(), NULL);

    if (result == 0) {
        std::cout << "[+] inject returned success (0)\n";
    } else {
        std::cout << "[-] inject returned failure (" << result << ")\n";
    }

    FreeLibrary(hLib);
    return (result == 0) ? 0 : 4;
}
