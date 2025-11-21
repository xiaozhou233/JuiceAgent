#ifndef JUICEAGENT_H
#define JUICEAGENT_H

#define INJECT_PATH_MAX 260

typedef struct _InjectionInfo {
    char BootstrapAPIPath[INJECT_PATH_MAX];
    char JuiceLoaderJarPath[INJECT_PATH_MAX];
    char EntryJarPath[INJECT_PATH_MAX];
    char EntryClass[INJECT_PATH_MAX];
    char EntryMethod[INJECT_PATH_MAX];
    char InjectionDir[INJECT_PATH_MAX];
    char JuiceLoaderLibraryPath[INJECT_PATH_MAX];
} InjectionInfoType;

extern InjectionInfoType InjectionInfo;

#endif // JUICEAGENT_H