#include "InjectorUtils.h"

/* Helpers: parse PE header to get Machine and check for ReflectiveLoader export */

/* return IMAGE_FILE_MACHINE_* value or 0 on error */
WORD get_pe_machine(LPVOID buffer, DWORD length) {
    if (!buffer || length < sizeof(IMAGE_DOS_HEADER)) return 0;
    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)buffer;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;
    if ((DWORD)dos->e_lfanew + sizeof(IMAGE_NT_HEADERS) > length) return 0;
    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((BYTE*)buffer + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return 0;
    return nt->FileHeader.Machine;
}

/* return TRUE if export table contains "ReflectiveLoader" */
BOOL has_reflective_loader_export(LPVOID buffer, DWORD length) {
    if (!buffer || length < sizeof(IMAGE_DOS_HEADER)) return FALSE;
    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)buffer;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return FALSE;
    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((BYTE*)buffer + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return FALSE;

    DWORD exportRVA = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    DWORD exportSize = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
    if (!exportRVA || exportSize == 0) return FALSE;

    // Need to convert RVA -> raw offset: iterate sections
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(nt);
    WORD numberOfSections = nt->FileHeader.NumberOfSections;
    DWORD exportOffset = 0;
    for (WORD i = 0; i < numberOfSections; ++i) {
        DWORD va = section[i].VirtualAddress;
        DWORD vs = section[i].Misc.VirtualSize;
        DWORD ptr = section[i].PointerToRawData;
        DWORD size = section[i].SizeOfRawData;
        if (exportRVA >= va && exportRVA < va + vs) {
            exportOffset = ptr + (exportRVA - va);
            break;
        }
    }
    if (!exportOffset) return FALSE;
    if ((DWORD)exportOffset + sizeof(IMAGE_EXPORT_DIRECTORY) > length) return FALSE;

    PIMAGE_EXPORT_DIRECTORY exp = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)buffer + exportOffset);
    if (exp->NumberOfNames == 0) return FALSE;

    // locate arrays
    DWORD addrOfNamesRVA = exp->AddressOfNames;
    DWORD addrOfNameOrdinalsRVA = exp->AddressOfNameOrdinals;
    if (!addrOfNamesRVA || !addrOfNameOrdinalsRVA) return FALSE;

    // find raw offsets for these arrays
    DWORD namesOffset = 0;
    DWORD ordinalsOffset = 0;
    for (WORD i = 0; i < numberOfSections; ++i) {
        DWORD va = section[i].VirtualAddress;
        DWORD vs = section[i].Misc.VirtualSize;
        DWORD ptr = section[i].PointerToRawData;
        if (addrOfNamesRVA >= va && addrOfNamesRVA < va + vs) {
            namesOffset = ptr + (addrOfNamesRVA - va);
        }
        if (addrOfNameOrdinalsRVA >= va && addrOfNameOrdinalsRVA < va + vs) {
            ordinalsOffset = ptr + (addrOfNameOrdinalsRVA - va);
        }
    }
    if (!namesOffset || !ordinalsOffset) return FALSE;
    if ((DWORD)namesOffset + exp->NumberOfNames * sizeof(DWORD) > length) return FALSE;
    if ((DWORD)ordinalsOffset + exp->NumberOfNames * sizeof(WORD) > length) return FALSE;

    // iterate names
    for (DWORD i = 0; i < exp->NumberOfNames; ++i) {
        DWORD nameRVA = *(DWORD*)((BYTE*)buffer + namesOffset + i * sizeof(DWORD));
        // convert nameRVA to raw offset
        DWORD nameOffset = 0;
        for (WORD s = 0; s < numberOfSections; ++s) {
            DWORD va = section[s].VirtualAddress;
            DWORD vs = section[s].Misc.VirtualSize;
            DWORD ptr = section[s].PointerToRawData;
            if (nameRVA >= va && nameRVA < va + vs) {
                nameOffset = ptr + (nameRVA - va);
                break;
            }
        }
        if (!nameOffset) continue;
        if ((DWORD)nameOffset >= length) continue;
        char *name = (char*)((BYTE*)buffer + nameOffset);
        // safe compare - ensure string is within buffer
        size_t maxlen = length - nameOffset;
        if (strncasecmp(name, "ReflectiveLoader", maxlen) == 0) return TRUE;
    }
    return FALSE;
}

/* Pretty-print machine */
const char* machine_to_str(WORD machine) {
    switch(machine) {
        case IMAGE_FILE_MACHINE_I386: return "x86 (IMAGE_FILE_MACHINE_I386)";
        case IMAGE_FILE_MACHINE_AMD64: return "x64 (IMAGE_FILE_MACHINE_AMD64)";
        case IMAGE_FILE_MACHINE_ARM: return "ARM";
        case IMAGE_FILE_MACHINE_ARM64: return "ARM64";
        default: return "Unknown";
    }
}