#ifndef INJECTOR_UTILS_H
#define INJECTOR_UTILS_H

#include "windows.h"

WORD get_pe_machine(LPVOID, DWORD);
BOOL has_reflective_loader_export(LPVOID, DWORD);
const char* machine_to_str(WORD);

#endif