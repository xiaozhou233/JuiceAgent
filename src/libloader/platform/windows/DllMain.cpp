#include <windows.h>

#include "JuiceAgent/Logger.hpp"
#include "Loader.hpp"


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH: {
            Logger::init("libloader.log");

            const char* runtime_dir = (const char*)lpReserved;
            spdlog::info("DllMain: DLL_PROCESS_ATTACH, runtime_dir: {}", runtime_dir ? runtime_dir : "NULL");

            JuiceAgent::Loader::entrypoint(runtime_dir);
            break;
        }

        case DLL_PROCESS_DETACH: {
            spdlog::info("DllMain: DLL_PROCESS_DETACH");
            break;
        }
    }
    return TRUE;
}
