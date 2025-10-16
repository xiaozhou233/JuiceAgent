#ifndef INJECTPARAMETERS_H
#define INJECTPARAMETERS_H

#include <windows.h>

#define INJECT_PATH_MAX 4096

typedef struct InjectParameters{
    char ConfigPath[INJECT_PATH_MAX];
} InjectParameters;

#endif