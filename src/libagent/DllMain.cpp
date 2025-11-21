#include <windows.h>
#include <JuiceAgent/Logger.hpp>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved ) {
    switch( fdwReason ) 
    { 
        case DLL_PROCESS_ATTACH:
            InitLogger();
            PLOGI << "DllMain: DLL_PROCESS_ATTACH";
            PLOGD.printf("lpReserved: %p", lpReserved);
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            PLOGI << "DllMain: DLL_PROCESS_DETACH";
            PLOGD.printf("lpReserved: %p", lpReserved);
            break;
    }
    return TRUE;
}