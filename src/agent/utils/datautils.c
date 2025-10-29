/*
Author: xiaozhou233 (xiaozhou233@vip.qq.com)
File: datautils.c 
Desc: data utils
*/


#include "datautils.h"
#include <stdio.h>
#include <string.h>
#include "tomlc17.h"

void safe_copy(char *dest, const char *src, size_t dest_size) {
    if (!dest || dest_size == 0) return;
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
}

bool ReadInjectionInfo(toml_result_t result, InjectParameters* params) {
    toml_datum_t InjectionTree = toml_seek(result.toptab, "Injection");

    // ======= BootstrapAPIPath =======
    toml_datum_t BootstrapAPIPath = toml_seek(InjectionTree, "BootstrapAPIPath");
    char BootstrapAPIPathStr[INJECT_PATH_MAX];
    if (BootstrapAPIPath.type != TOML_STRING) {
        log_error("BootstrapAPIPath is not a string.");
        return false;
    }
    if (BootstrapAPIPath.u.s == NULL || strlen(BootstrapAPIPath.u.s) == 0) {
        sprintf(BootstrapAPIPathStr, "%s\\bootstrap-api.jar", params->ConfigDir);
    } else {
        sprintf(BootstrapAPIPathStr, "%s", BootstrapAPIPath.u.s);
    }
    safe_copy(InjectionInfo.BootstrapAPIPath, BootstrapAPIPathStr, INJECT_PATH_MAX);
    // ======= BootstrapAPIPath =======

    // ======= JuiceLoaderJarPath =======
    toml_datum_t JuiceLoaderJarPath = toml_seek(InjectionTree, "JuiceLoaderJarPath");
    char JuiceLoaderJarPathStr[INJECT_PATH_MAX];
    if (JuiceLoaderJarPath.type != TOML_STRING) {
        log_error("JuiceLoaderJarPath is not a string.");
        return false;
    }
    if (JuiceLoaderJarPath.u.s == NULL || strlen(JuiceLoaderJarPath.u.s) == 0) {
        sprintf(JuiceLoaderJarPathStr, "%s\\JuiceLoader.jar", params->ConfigDir);
    } else {
        sprintf(JuiceLoaderJarPathStr, "%s", JuiceLoaderJarPath.u.s);
    }
    safe_copy(InjectionInfo.JuiceLoaderJarPath, JuiceLoaderJarPathStr, INJECT_PATH_MAX);
    // ======= JuiceLoaderJarPath =======

    // ======= Entry =======
    toml_datum_t EntryJarPath = toml_seek(InjectionTree, "EntryJarPath");
    char EntryJarPathStr[INJECT_PATH_MAX];
    if (EntryJarPath.type != TOML_STRING) {
        log_error("EntryJarPath is not a string.");
        return false;
    }
    if (EntryJarPath.u.s == NULL || strlen(EntryJarPath.u.s) == 0) {
        sprintf(EntryJarPathStr, "%s\\entry.jar", params->ConfigDir);
    } else {
        sprintf(EntryJarPathStr, "%s", EntryJarPath.u.s);
    }
    safe_copy(InjectionInfo.EntryJarPath, EntryJarPathStr, INJECT_PATH_MAX);

    toml_datum_t EntryClass = toml_seek(InjectionTree, "EntryClass");
    if (EntryClass.type != TOML_STRING) {
        log_error("EntryClass is not a string.");
        return false;
    }
    if (EntryClass.u.s == NULL || strlen(EntryClass.u.s) == 0) {
        safe_copy(InjectionInfo.EntryClass, "cn.xiaozhou233.juiceloader.entry.Entry", INJECT_PATH_MAX);
    } else {
        safe_copy(InjectionInfo.EntryClass, EntryClass.u.s, INJECT_PATH_MAX);
    }

    toml_datum_t EntryMethod = toml_seek(InjectionTree, "EntryMethod");
    if (EntryMethod.type != TOML_STRING) {
        log_error("EntryMethod is not a string.");
        return false;
    }
    if (EntryMethod.u.s == NULL || strlen(EntryMethod.u.s) == 0) {
        safe_copy(InjectionInfo.EntryMethod, "start", INJECT_PATH_MAX);
    } else {
        safe_copy(InjectionInfo.EntryMethod, EntryMethod.u.s, INJECT_PATH_MAX);
    }
    // ======= Entry =======
    return true;
}
