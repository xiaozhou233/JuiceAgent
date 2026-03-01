#ifndef JUICELOADER_H
#define JUICELOADER_H

#include <jni.h>
#include <jvmti.h>

typedef struct _JuiceLoaderNative {
    jvmtiEnv *jvmti;
} JuiceLoaderNativeType;
extern JuiceLoaderNativeType JuiceLoaderNative;

typedef struct {
    char *name;
    unsigned char *bytes;
    int len;
} ClassBytesCacheType;

typedef struct {
    ClassBytesCacheType *arr;
    int size;
    int capacity;
} RetransformClassCacheType;

extern RetransformClassCacheType RetransformClassCache;

extern "C" __declspec(dllexport)
int InitJuiceLoader(JNIEnv *env, jvmtiEnv *jvmti);

#endif