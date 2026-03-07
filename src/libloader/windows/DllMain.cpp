#include <windows.h>
#include <JuiceAgent/Logger.hpp>
#include <libloader.hpp>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved ) {
    switch( fdwReason ) 
    { 
        case DLL_PROCESS_ATTACH: {
            InitLogger();
            PLOGI << "DllMain: DLL_PROCESS_ATTACH";

            const char* runtime_dir = (const char*)lpReserved;

            libloader::entrypoint(runtime_dir);

            break;
        }

        case DLL_THREAD_ATTACH: {
            break;
        }

        case DLL_THREAD_DETACH: {
            break;
        }
            

        case DLL_PROCESS_DETACH: {
            PLOGI << "DllMain: DLL_PROCESS_DETACH";
            break;
        }
    }
    return TRUE;
}