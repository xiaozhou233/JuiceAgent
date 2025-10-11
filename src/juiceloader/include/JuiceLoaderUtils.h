#ifndef JUICELOADER_UTILS_H
#define JUICELOADER_UTILS_H

#include "jvmti.h"

char* get_method_fullname(jvmtiEnv*, JNIEnv*, jthread, jmethodID);

#endif