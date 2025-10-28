#ifndef JUICELOADER_H
#define JUICELOADER_H

#include <jni.h>
#include <jvmti.h>

struct _JuiceLoaderNative {
    JavaVM *jvm;
    jvmtiEnv *jvmti;
} JuiceLoaderNative;


#endif