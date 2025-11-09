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

#define C_EXPORT extern "C" __declspec(dllexport)

int InitJuiceLoader(JNIEnv *env, jvmtiEnv *jvmti);

void JNICALL ClassFileLoadHook(
        jvmtiEnv* jvmti_env,
        JNIEnv* jni_env,
        jclass class_being_redefined,
        jobject loader,
        const char* name,
        jobject protection_domain,
        jint class_data_len,
        const unsigned char* classbytes,
        jint* new_class_data_len,
        unsigned char** new_classbytes);

// JNI Functions
// public native static boolean init();
JNIEXPORT jboolean JNICALL loader_init(JNIEnv *env, jclass loader_class);
// public native static boolean injectJar(String jarPath);
JNIEXPORT jboolean JNICALL loader_injectJar(JNIEnv *env, jclass loader_class, jstring jarPath);
// public native static boolean redefineClass(Class<?> clazz, byte[] classBytes, int length);
JNIEXPORT jboolean JNICALL loader_redefineClass_clazz(JNIEnv *env, jclass loader_class, jclass clazz, jbyteArray classBytes, jint length);
// public native static boolean redefineClass(String className, byte[] classBytes, int length);
JNIEXPORT jboolean JNICALL loader_redefineClass_className(JNIEnv *env, jclass loader_class, jstring className, jbyteArray classBytes, jint length);
// public native static Class<?>[] getLoadedClasses();
JNIEXPORT jobjectArray JNICALL loader_getLoadedClasses(JNIEnv *env, jclass loader_class);
// public static native byte[] getClassBytes(Class<?> clazz);
JNIEXPORT jbyteArray JNICALL loader_getClassBytes(JNIEnv *env, jclass loader_class, jclass clazz);
// public static native byte[] getClassBytesByName(String className);
JNIEXPORT jbyteArray JNICALL loader_getClassBytesByName(JNIEnv *env, jclass loader_class, jstring className);
// public static native boolean retransformClass(Class<?> clazz, byte[] classBytes, int length);
JNIEXPORT jboolean JNICALL loader_retransformClass(JNIEnv *env, jclass loader_class, jclass clazz, jbyteArray classBytes, jint length);
// public static native boolean retransformClassByName(String className, byte[] classBytes, int length);
JNIEXPORT jboolean JNICALL loader_retransformClassByName(JNIEnv *env, jclass loader_class, jstring className, jbyteArray classBytes, jint length);
// public static native Class<?> getClassByName(String className);
JNIEXPORT jclass JNICALL loader_getClassByName(JNIEnv *env, jclass loader_class, jstring className);
JNIEXPORT jobject JNICALL loader_nativeGetThreadByName (JNIEnv* env, jclass, jstring jName);
JNIEXPORT jobject JNICALL loader_nativeInjectJarToThread (JNIEnv* env, jclass, jobject threadObj, jstring jJarPath);
#endif