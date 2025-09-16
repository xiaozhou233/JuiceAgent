#include "windows.h"
#include <stdio.h>
#include <stdlib.h>
#include "LoadLibraryR.h"
#include "InjectorNative.h"

#define WIN_X64
#define BREAK_WITH_ERROR( e ) { printf( "[-] %s. Error=%d", e, GetLastError() ); break; }

int inject(int pid, char *path){
    HANDLE hFile          = NULL;
	HANDLE hModule        = NULL;
	HANDLE hProcess       = NULL;
	HANDLE hToken         = NULL;
	LPVOID lpBuffer       = NULL;
	DWORD dwLength        = 0;
	DWORD dwBytesRead     = 0;
	DWORD dwProcessId     = 0;
	TOKEN_PRIVILEGES priv = {0};
	char * cpDllFile  = NULL;

    cpDllFile = path;
    dwProcessId = pid;

    do {
		hFile = CreateFileA( cpDllFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
		if( hFile == INVALID_HANDLE_VALUE )
			BREAK_WITH_ERROR( "Failed to open the DLL file" );

		dwLength = GetFileSize( hFile, NULL );
		if( dwLength == INVALID_FILE_SIZE || dwLength == 0 )
			BREAK_WITH_ERROR( "Failed to get the DLL file size" );

		lpBuffer = HeapAlloc( GetProcessHeap(), 0, dwLength );
		if( !lpBuffer )
			BREAK_WITH_ERROR( "Failed to get the DLL file size" );

		if( ReadFile( hFile, lpBuffer, dwLength, &dwBytesRead, NULL ) == FALSE )
			BREAK_WITH_ERROR( "Failed to alloc a buffer!" );

		if( OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken ) )
		{
			priv.PrivilegeCount           = 1;
			priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		
			if( LookupPrivilegeValue( NULL, SE_DEBUG_NAME, &priv.Privileges[0].Luid ) )
				AdjustTokenPrivileges( hToken, FALSE, &priv, 0, NULL, NULL );

			CloseHandle( hToken );
		}

		hProcess = OpenProcess( PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, dwProcessId );
		if( !hProcess )
			BREAK_WITH_ERROR( "Failed to open the target process" );

		hModule = LoadRemoteLibraryR( hProcess, lpBuffer, dwLength, NULL );
		if( !hModule )
			BREAK_WITH_ERROR( "Failed to inject the DLL" );

		printf( "[+] Injected the '%s' DLL into process %d.", cpDllFile, dwProcessId );
		
		WaitForSingleObject( hModule, -1 );

	} while( 0 );

	if( lpBuffer )
		HeapFree( GetProcessHeap(), 0, lpBuffer );

	if( hProcess )
		CloseHandle( hProcess );

	return 0;
}

int inject_pid(int pid){
    const char* dir = getenv("JuiceAgent_Dir");
    if(!dir) return -1;
    
    char path[MAX_PATH];
    sprintf(path, "%s\\libagent.dll", dir);
    return inject(pid, path);
}

JNIEXPORT jboolean JNICALL Java_cn_xiaozhou233_orangex_injector_InjectorNative_inject
  (JNIEnv *env, jobject obj, jint pid, jstring path){
    const char* nativePath = (*env)->GetStringUTFChars(env, path, 0);
    int ret = inject(pid, (char*)nativePath);
    (*env)->ReleaseStringUTFChars(env, path, nativePath);

    if(ret == 0)
        return JNI_TRUE;
    return JNI_FALSE;
}
