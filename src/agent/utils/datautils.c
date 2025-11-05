/*
Author: xiaozhou233 (xiaozhou233@vip.qq.com)
File: datautils.c 
Desc: data utils
*/


#include "datautils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tomlc17.h"
#include "juiceloader.h"

void safe_copy(char *dest, const char *src, size_t dest_size) {
    if (!dest || dest_size == 0) return;
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
}
static void GetTomlStringOrDefault(toml_datum_t tableDatum, const char* key, const char* defaultVal, char* outBuf, size_t bufSize) {
    toml_datum_t datum = toml_seek(tableDatum, key);
    if (datum.type != TOML_STRING || !datum.u.s || datum.u.s[0] == '\0') {
        safe_copy(outBuf, defaultVal, bufSize);
    } else {
        safe_copy(outBuf, datum.u.s, bufSize);
    }
}

bool ReadInjectionInfo(toml_result_t result, InjectParameters* params) {
    if (!result.ok) return false;

    toml_datum_t InjectionTree = toml_seek(result.toptab, "Injection");
    if (InjectionTree.type != TOML_TABLE) {
        log_error("Injection section missing");
        return false;
    }

    char tmp[INJECT_PATH_MAX];

    snprintf(tmp, sizeof(tmp), "%s\\JuiceLoader.jar", params->ConfigDir);
    GetTomlStringOrDefault(InjectionTree, "JuiceLoaderJarPath", tmp, InjectionInfo.JuiceLoaderJarPath, INJECT_PATH_MAX);

    snprintf(tmp, sizeof(tmp), "%s\\entry.jar", params->ConfigDir);
    GetTomlStringOrDefault(InjectionTree, "EntryJarPath", tmp, InjectionInfo.EntryJarPath, INJECT_PATH_MAX);

    GetTomlStringOrDefault(InjectionTree, "EntryClass", "cn.xiaozhou233.juiceloader.entry.Entry", InjectionInfo.EntryClass, INJECT_PATH_MAX);
    GetTomlStringOrDefault(InjectionTree, "EntryMethod", "start", InjectionInfo.EntryMethod, INJECT_PATH_MAX);

    snprintf(tmp, sizeof(tmp), "%s\\injection", params->ConfigDir);
    GetTomlStringOrDefault(InjectionTree, "InjectionDir", tmp, InjectionInfo.InjectionDir, INJECT_PATH_MAX);

    return true;
}
