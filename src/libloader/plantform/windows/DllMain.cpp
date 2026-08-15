#include <windows.h>

#include "JuiceAgent/Logger.hpp"
#include "Loader.hpp"

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH: {
            DisableThreadLibraryCalls(hinstDLL);
            Logger::init("libloader.log");

            spdlog::info("DllMain: DLL_PROCESS_ATTACH");

            // Get the runtime directory from the lpReserved parameter
            const char* runtime_dir = (const char*)lpReserved; 
            spdlog::info("DllMain: runtime_dir: {}", runtime_dir);

            JuiceAgent::Loader::entrypoint(runtime_dir);

            break;
        }

        case DLL_THREAD_ATTACH: {
            break;
        }

        case DLL_THREAD_DETACH: {
            break;
        }

        case DLL_PROCESS_DETACH: {
            spdlog::info("DllMain: DLL_PROCESS_DETACH");
            break;
        }
    }
    return TRUE;
}
