#ifndef DATAUTILS_H
#define DATAUTILS_H

#include <string.h>
#include "InjectParameters.h"
#include "tomlc17.h"
#include "log.h"

typedef struct _InjectionInfo {
    char BootstrapAPIPath[INJECT_PATH_MAX];
    char JuiceLoaderJarPath[INJECT_PATH_MAX];
    char EntryJarPath[INJECT_PATH_MAX];
    char EntryClass[INJECT_PATH_MAX];
    char EntryMethod[INJECT_PATH_MAX];
    char InjectionDir[INJECT_PATH_MAX];
} InjectionInfoType;
extern InjectionInfoType InjectionInfo;

void safe_copy(char *dest, const char *src, size_t dest_size);
bool ReadInjectionInfo(toml_result_t result, InjectParameters* params);

#endif // DATAUTILS_H