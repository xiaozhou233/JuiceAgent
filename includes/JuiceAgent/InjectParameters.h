#ifndef INJECTPARAMETERS_H
#define INJECTPARAMETERS_H

#include <windows.h>

#define INJECT_PATH_MAX 4096

typedef struct InjectParameters{
    char JuiceLoaderJarPath[INJECT_PATH_MAX];
    char BootstrapApiPath[INJECT_PATH_MAX];
    // Optional: 
    char JuiceLoaderLibPath[INJECT_PATH_MAX];
    // Optional:
    char EntryJarPath[INJECT_PATH_MAX];
} InjectParameters;

#endif