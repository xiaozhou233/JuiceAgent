#include <windows.h>
#include <JuiceAgent/Logger.hpp>
#include <libagent.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved ) {
    switch( fdwReason ) 
    { 
        case DLL_PROCESS_ATTACH: {
            InitLogger();
            PLOGI << "DllMain: DLL_PROCESS_ATTACH";
            PLOGD.printf("lpReserved: %p", lpReserved);

            const char* runtime_dir = (const char*)lpReserved;
            PLOGD << "runtime_dir: " << runtime_dir;

            InitJuiceAgent(runtime_dir);

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
            PLOGD.printf("lpReserved: %p", lpReserved);
            break;
        }
    }
    return TRUE;
}