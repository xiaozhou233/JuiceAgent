#ifndef INJECTPARAMETERS_H
#define INJECTPARAMETERS_H

#include <windows.h>

typedef struct InjectParameters{
    char *JuiceLoaderJarPath;
    // Optional: 
    char *JuiceLoaderLibPath;
    // Optional:
    char *EntryJarPath;
} InjectParameters;

#endif