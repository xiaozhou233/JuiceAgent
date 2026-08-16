#include <windows.h>

#include "JuiceAgent/Logger.hpp"
#include "Loader.hpp"

static bool g_loaded_via_injection = false;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH: {
            if (lpReserved == NULL) {
                return TRUE;
            }

            g_loaded_via_injection = true;
            DisableThreadLibraryCalls(hinstDLL);
            Logger::init("libloader.log");

            const char* runtime_dir = (const char*)lpReserved;
            spdlog::info("DllMain: DLL_PROCESS_ATTACH, runtime_dir: {}", runtime_dir);

            JuiceAgent::Loader::entrypoint(runtime_dir);
            break;
        }

        case DLL_PROCESS_DETACH: {
            if (g_loaded_via_injection) {
                spdlog::info("DllMain: DLL_PROCESS_DETACH");
            }
            break;
        }
    }
    return TRUE;
}
